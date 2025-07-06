#include "nysys.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <process.h>
#include <stdexcept>
#include <string>
#include <utility>

#include "helper/json_structure.hpp"
#include "internal.hpp"

class HandleWrapper {
public:
  explicit HandleWrapper(HANDLE handle = nullptr) noexcept : m_handle(handle) {}

  ~HandleWrapper() noexcept { Close(); }

  HandleWrapper(HandleWrapper &&other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) {}

  HandleWrapper &operator=(HandleWrapper &&other) noexcept {
    if (this != &other) {
      Close();
      m_handle = std::exchange(other.m_handle, nullptr);
    }
    return *this;
  }

  HandleWrapper(const HandleWrapper &) = delete;
  HandleWrapper &operator=(const HandleWrapper &) = delete;

  [[nodiscard]] HANDLE get() const noexcept { return m_handle; }

  [[nodiscard]] HANDLE release() noexcept { return std::exchange(m_handle, nullptr); }

  void reset(HANDLE newHandle = nullptr) noexcept {
    Close();
    m_handle = newHandle;
  }

  [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }

  [[nodiscard]] bool IsValid() const noexcept { return m_handle && m_handle != INVALID_HANDLE_VALUE; }

private:
  void Close() noexcept {
    if (IsValid()) {
      CloseHandle(m_handle);
      m_handle = nullptr;
    }
  }

  HANDLE m_handle;
};

class MonitorContext {
public:
  std::atomic<bool> isRunning{false};
  std::atomic<int32_t> updateInterval{nysys::DEFAULT_UPDATE_INTERVAL_MS};
  std::atomic<bool> isFirstRun{true};
  std::atomic<bool> shouldStop{false};
  std::atomic<size_t> cycleCount{0};
  std::atomic<nysys::MonitoringError> lastError{nysys::MonitoringError::Success};

  HandleWrapper monitorThread;
  HandleWrapper stopEvent;
  mutable std::mutex dataMutex;
  mutable std::mutex callbackMutex;
  mutable std::mutex errorMutex;

  NysysCallback cCallback{nullptr};
  std::function<void(const std::string &)> cppCallback;

  nysys::StaticInfo staticInfo;
  nysys::DynamicInfo dynamicInfo;

  std::chrono::steady_clock::time_point startTime;
  std::chrono::steady_clock::time_point lastUpdateTime;

  MonitorContext() = default;

  MonitorContext(const MonitorContext &) = delete;
  MonitorContext &operator=(const MonitorContext &) = delete;

  void Reset() noexcept {
    std::lock_guard<std::mutex> lock(dataMutex);
    staticInfo.Reset();
    dynamicInfo.Reset();
    isFirstRun = true;
    shouldStop = false;
    cycleCount = 0;
    lastError = nysys::MonitoringError::Success;
    startTime = {};
    lastUpdateTime = {};
  }

  [[nodiscard]] static bool IsValidInterval(int32_t intervalMs) noexcept {
    return intervalMs >= nysys::MIN_UPDATE_INTERVAL_MS && intervalMs <= 3600000;
  }

  [[nodiscard]] nysys::MonitoringError SetUpdateInterval(int32_t intervalMs) noexcept {
    if (!IsValidInterval(intervalMs)) {
      SetLastError(nysys::MonitoringError::InvalidParameter);
      return nysys::MonitoringError::InvalidParameter;
    }
    updateInterval = intervalMs;
    return nysys::MonitoringError::Success;
  }

  void SetLastError(nysys::MonitoringError error) noexcept {
    std::lock_guard<std::mutex> lock(errorMutex);
    lastError = error;
  }

  [[nodiscard]] nysys::MonitoringError GetLastError() const noexcept {
    std::lock_guard<std::mutex> lock(errorMutex);
    return lastError;
  }

  [[nodiscard]] nysys::MonitoringError InvokeCallbacks(const std::string &jsonData) const noexcept {
    if (jsonData.empty()) {
      return nysys::MonitoringError::InvalidParameter;
    }

    std::lock_guard<std::mutex> lock(callbackMutex);

    bool callbackFailed = false;

    if (cCallback) {
      try {
        cCallback(jsonData.c_str());
      } catch (...) {
        callbackFailed = true;
      }
    }

    if (cppCallback) {
      try {
        cppCallback(jsonData);
      } catch (...) {
        callbackFailed = true;
      }
    }

    return callbackFailed ? nysys::MonitoringError::CallbackExecutionFailed : nysys::MonitoringError::Success;
  }

  void InitializeSession() noexcept {
    startTime = std::chrono::steady_clock::now();
    lastUpdateTime = startTime;
    cycleCount = 0;
    SetLastError(nysys::MonitoringError::Success);
  }

  void IncrementCycle() noexcept {
    ++cycleCount;
    lastUpdateTime = std::chrono::steady_clock::now();
  }

  [[nodiscard]] std::chrono::milliseconds GetUptime() const noexcept {
    if (startTime.time_since_epoch().count() == 0) {
      return std::chrono::milliseconds{0};
    }
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
  }

  [[nodiscard]] bool ShouldStop() const noexcept { return shouldStop || !isRunning; }

  void RequestStop() noexcept { shouldStop = true; }
};

static MonitorContext g_MonitorContext;

static nysys::MonitoringError CollectStaticInfo(nysys::StaticInfo &staticInfo) noexcept {
  try {
    staticInfo.cpuList = nysys::GetCPUList();
    if (!staticInfo.cpuList) {
      return nysys::MonitoringError::DataCollectionFailed;
    }

    staticInfo.gpuList = nysys::GetGPUList();
    if (!staticInfo.gpuList) {
      return nysys::MonitoringError::DataCollectionFailed;
    }

    staticInfo.mbInfo = nysys::GetMotherboardInfo();
    if (!staticInfo.mbInfo) {
      return nysys::MonitoringError::DataCollectionFailed;
    }

    staticInfo.audioList = nysys::GetAudioDeviceList();
    if (!staticInfo.audioList) {
      return nysys::MonitoringError::DataCollectionFailed;
    }

    staticInfo.monitorList = nysys::GetMonitorList();
    if (!staticInfo.monitorList) {
      return nysys::MonitoringError::DataCollectionFailed;
    }

    return nysys::MonitoringError::Success;
  } catch (...) {
    staticInfo.Reset();
    return nysys::MonitoringError::DataCollectionFailed;
  }
}

static nysys::MonitoringError CollectDynamicInfo(nysys::DynamicInfo &dynamicInfo) noexcept {
  try {
    dynamicInfo.memInfo = nysys::GetMemoryInfo();
    if (!dynamicInfo.memInfo) {
      return nysys::MonitoringError::DataCollectionFailed;
    }

    dynamicInfo.storageList = nysys::GetStorageList();
    if (!dynamicInfo.storageList) {
      return nysys::MonitoringError::DataCollectionFailed;
    }

    dynamicInfo.networkList = nysys::GetNetworkAdapterList();
    if (!dynamicInfo.networkList) {
      return nysys::MonitoringError::DataCollectionFailed;
    }

    dynamicInfo.batteryInfo = nysys::GetBatteryInfo();

    return nysys::MonitoringError::Success;
  } catch (...) {
    dynamicInfo.Reset();
    return nysys::MonitoringError::DataCollectionFailed;
  }
}

static std::optional<std::string> GenerateJsonSafely(const nysys::StaticInfo &staticInfo,
                                                     const nysys::DynamicInfo &dynamicInfo) noexcept {
  try {
    if (!staticInfo.IsComplete() || !dynamicInfo.IsEssentialComplete()) {
      return std::nullopt;
    }

    json::JsonConfig config{};
    auto jsonResult = json::GenerateSystemInfo(
        staticInfo.gpuList.get(), staticInfo.mbInfo.get(), staticInfo.cpuList.get(), dynamicInfo.memInfo.get(),
        dynamicInfo.storageList.get(), dynamicInfo.networkList.get(), staticInfo.audioList.get(),
        dynamicInfo.batteryInfo.get(), staticInfo.monitorList.get(), config);

    return jsonResult.has_value() ? std::make_optional(std::move(jsonResult.value())) : std::nullopt;
  } catch (...) {
    return std::nullopt;
  }
}

static unsigned __stdcall monitoring_thread(void *) {
  g_MonitorContext.InitializeSession();

  while (g_MonitorContext.isRunning) {
    if (WaitForSingleObject(g_MonitorContext.stopEvent.get(), 0) == WAIT_OBJECT_0)
      break;

    if (g_MonitorContext.isFirstRun) {
      std::lock_guard<std::mutex> lock(g_MonitorContext.dataMutex);
      auto staticResult = CollectStaticInfo(g_MonitorContext.staticInfo);
      if (staticResult == nysys::MonitoringError::Success) {
        g_MonitorContext.isFirstRun = false;
      } else {
        g_MonitorContext.SetLastError(staticResult);
      }
    }

    nysys::MonitoringError dynamicResult = nysys::MonitoringError::DataCollectionFailed;
    {
      std::lock_guard<std::mutex> lock(g_MonitorContext.dataMutex);
      dynamicResult = CollectDynamicInfo(g_MonitorContext.dynamicInfo);
    }

    if (dynamicResult == nysys::MonitoringError::Success && !g_MonitorContext.isFirstRun) {
      std::optional<std::string> jsonOutput;
      {
        std::lock_guard<std::mutex> lock(g_MonitorContext.dataMutex);
        jsonOutput = GenerateJsonSafely(g_MonitorContext.staticInfo, g_MonitorContext.dynamicInfo);
      }

      if (jsonOutput.has_value()) {
        auto callbackResult = g_MonitorContext.InvokeCallbacks(jsonOutput.value());
        if (callbackResult != nysys::MonitoringError::Success) {
          g_MonitorContext.SetLastError(callbackResult);
        }
      } else {
        g_MonitorContext.SetLastError(nysys::MonitoringError::JsonGenerationFailed);
      }
    } else if (dynamicResult != nysys::MonitoringError::Success) {
      g_MonitorContext.SetLastError(dynamicResult);
    }

    {
      std::lock_guard<std::mutex> lock(g_MonitorContext.dataMutex);
      g_MonitorContext.dynamicInfo.Reset();
    }

    g_MonitorContext.IncrementCycle();

    const auto currentInterval = g_MonitorContext.updateInterval.load();
    Sleep(static_cast<DWORD>(currentInterval));
  }

  return 0;
}

BOOL start_monitoring(int32_t updateIntervalMs) {
  if (g_MonitorContext.isRunning) {
    g_MonitorContext.SetLastError(nysys::MonitoringError::AlreadyRunning);
    return FALSE;
  }

  auto intervalResult = g_MonitorContext.SetUpdateInterval(updateIntervalMs);
  if (intervalResult != nysys::MonitoringError::Success) {
    g_MonitorContext.SetLastError(intervalResult);
    return FALSE;
  }

  try {
    g_MonitorContext.isFirstRun = true;

    HANDLE stopEventHandle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!stopEventHandle) {
      g_MonitorContext.SetLastError(nysys::MonitoringError::SystemResourceError);
      return FALSE;
    }
    g_MonitorContext.stopEvent.reset(stopEventHandle);

    g_MonitorContext.Reset();

    HANDLE threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, monitoring_thread, nullptr, 0, nullptr));
    if (!threadHandle) {
      g_MonitorContext.stopEvent.reset();
      g_MonitorContext.SetLastError(nysys::MonitoringError::ThreadCreationFailed);
      return FALSE;
    }
    g_MonitorContext.monitorThread.reset(threadHandle);

    g_MonitorContext.isRunning = true;
    return TRUE;
  } catch (...) {
    g_MonitorContext.stopEvent.reset();
    g_MonitorContext.monitorThread.reset();
    g_MonitorContext.isRunning = false;
    g_MonitorContext.SetLastError(nysys::MonitoringError::UnknownError);
    return FALSE;
  }
}

void stop_monitoring(void) {
  if (!g_MonitorContext.isRunning)
    return;

  g_MonitorContext.RequestStop();

  if (g_MonitorContext.stopEvent) {
    SetEvent(g_MonitorContext.stopEvent.get());
  }

  if (g_MonitorContext.monitorThread) {
    const DWORD waitResult = WaitForSingleObject(g_MonitorContext.monitorThread.get(), nysys::MAX_THREAD_WAIT_MS);

    if (waitResult == WAIT_TIMEOUT) {
      TerminateThread(g_MonitorContext.monitorThread.get(), 1);
      g_MonitorContext.SetLastError(nysys::MonitoringError::ThreadTerminationFailed);
    } else if (waitResult == WAIT_FAILED) {
      g_MonitorContext.SetLastError(nysys::MonitoringError::SystemResourceError);
    }
  }

  g_MonitorContext.isRunning = false;

  g_MonitorContext.Reset();
  g_MonitorContext.monitorThread.reset();
  g_MonitorContext.stopEvent.reset();

  {
    std::lock_guard<std::mutex> lock(g_MonitorContext.callbackMutex);
    g_MonitorContext.cCallback = nullptr;
    g_MonitorContext.cppCallback = nullptr;
  }
}

void set_update_interval(int32_t updateIntervalMs) {
  auto result = g_MonitorContext.SetUpdateInterval(updateIntervalMs);
  if (result != nysys::MonitoringError::Success) {
    g_MonitorContext.SetLastError(result);
  }
}

void set_callback(NysysCallback callback) {
  std::lock_guard<std::mutex> lock(g_MonitorContext.callbackMutex);
  g_MonitorContext.cCallback = callback;

  if (callback != nullptr) {
    auto lastError = g_MonitorContext.GetLastError();
    if (lastError == nysys::MonitoringError::CallbackFailed) {
      g_MonitorContext.SetLastError(nysys::MonitoringError::Success);
    }
  }
}

BOOL is_monitoring(void) { return g_MonitorContext.isRunning ? TRUE : FALSE; }

namespace nysys {

bool StartMonitoring(int32_t updateIntervalMs) {
  auto intervalResult = g_MonitorContext.SetUpdateInterval(updateIntervalMs);
  if (intervalResult != MonitoringError::Success) {
    throw MonitoringException(intervalResult, "Invalid update interval: " + std::to_string(updateIntervalMs) + "ms");
  }

  bool result = start_monitoring(updateIntervalMs) != FALSE;
  if (!result) {
    auto lastError = g_MonitorContext.GetLastError();
    if (lastError != MonitoringError::Success) {
      throw MonitoringException(lastError, "Failed to start monitoring");
    }
  }
  return result;
}

void StopMonitoring() noexcept { stop_monitoring(); }

void SetUpdateInterval(int32_t updateIntervalMs) {
  auto result = g_MonitorContext.SetUpdateInterval(updateIntervalMs);
  if (result != MonitoringError::Success) {
    throw MonitoringException(result, "Invalid update interval: " + std::to_string(updateIntervalMs) + "ms");
  }
  set_update_interval(updateIntervalMs);
}

void SetCallback(const std::function<void(const std::string &)> &callback) {
  std::lock_guard<std::mutex> lock(g_MonitorContext.callbackMutex);
  g_MonitorContext.cppCallback = callback;

  if (callback) {
    auto lastError = g_MonitorContext.GetLastError();
    if (lastError == MonitoringError::CallbackFailed) {
      g_MonitorContext.SetLastError(MonitoringError::Success);
    }
  }
}

bool IsMonitoring() noexcept { return g_MonitorContext.isRunning; }

MonitoringError GetLastError() noexcept { return g_MonitorContext.GetLastError(); }

std::chrono::milliseconds GetUptime() noexcept { return g_MonitorContext.GetUptime(); }

}  // namespace nysys