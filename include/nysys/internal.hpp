#ifndef INTERNAL_HPP
#define INTERNAL_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include "main/audio_info.hpp"
#include "main/battery_info.hpp"
#include "main/cpu_info.hpp"
#include "main/gpu_info.hpp"
#include "main/memory_info.hpp"
#include "main/monitor_info.hpp"
#include "main/motherboard_info.hpp"
#include "main/network_info.hpp"
#include "main/storage_info.hpp"

namespace nysys {

enum class MonitoringError {
  Success = 0,
  InvalidParameter,
  InvalidInterval,
  AlreadyRunning,
  NotRunning,
  ThreadCreationFailed,
  ThreadTerminationFailed,
  EventCreationFailed,
  SystemResourceError,
  DataCollectionFailed,
  JsonGenerationFailed,
  CallbackFailed,
  CallbackExecutionFailed,
  UnknownError
};

[[nodiscard]] constexpr std::string_view ToString(MonitoringError error) noexcept {
  switch (error) {
    case MonitoringError::Success:
      return "Success";
    case MonitoringError::InvalidParameter:
      return "Invalid parameter";
    case MonitoringError::InvalidInterval:
      return "Invalid interval";
    case MonitoringError::AlreadyRunning:
      return "Monitoring already running";
    case MonitoringError::NotRunning:
      return "Monitoring not running";
    case MonitoringError::ThreadCreationFailed:
      return "Thread creation failed";
    case MonitoringError::ThreadTerminationFailed:
      return "Thread termination failed";
    case MonitoringError::EventCreationFailed:
      return "Event creation failed";
    case MonitoringError::SystemResourceError:
      return "System resource error";
    case MonitoringError::DataCollectionFailed:
      return "Data collection failed";
    case MonitoringError::JsonGenerationFailed:
      return "JSON generation failed";
    case MonitoringError::CallbackFailed:
      return "Callback failed";
    case MonitoringError::CallbackExecutionFailed:
      return "Callback execution failed";
    case MonitoringError::UnknownError:
      return "Unknown error";
    default:
      return "Unknown error";
  }
}

class MonitoringException : public std::runtime_error {
public:
  explicit MonitoringException(MonitoringError errorCode)
      : std::runtime_error(std::string("Monitoring Error: ") + std::string(ToString(errorCode))),
        m_errorCode(errorCode) {}

  explicit MonitoringException(MonitoringError errorCode, std::string_view details)
      : std::runtime_error(std::string("Monitoring Error: ") + std::string(ToString(errorCode)) + " - " +
                           std::string(details)),
        m_errorCode(errorCode) {}

  [[nodiscard]] MonitoringError GetErrorCode() const noexcept { return m_errorCode; }

private:
  MonitoringError m_errorCode;
};

struct StaticInfo {
  std::unique_ptr<GPUList> gpuList;
  std::unique_ptr<MotherboardInfo> mbInfo;
  std::unique_ptr<CPUList> cpuList;
  std::unique_ptr<AudioList> audioList;
  std::unique_ptr<MonitorList> monitorList;

  StaticInfo() = default;

  StaticInfo(StaticInfo &&) = default;
  StaticInfo &operator=(StaticInfo &&) = default;

  StaticInfo(const StaticInfo &) = delete;
  StaticInfo &operator=(const StaticInfo &) = delete;

  [[nodiscard]] bool IsComplete() const noexcept { return gpuList && mbInfo && cpuList && audioList && monitorList; }

  [[nodiscard]] bool HasGPUInfo() const noexcept { return static_cast<bool>(gpuList); }

  [[nodiscard]] bool HasMotherboardInfo() const noexcept { return static_cast<bool>(mbInfo); }

  [[nodiscard]] bool HasCPUInfo() const noexcept { return static_cast<bool>(cpuList); }

  [[nodiscard]] bool HasAudioInfo() const noexcept { return static_cast<bool>(audioList); }

  [[nodiscard]] bool HasMonitorInfo() const noexcept { return static_cast<bool>(monitorList); }

  [[nodiscard]] double GetCompletionPercentage() const noexcept {
    constexpr int totalComponents = 5;
    int completedComponents = 0;
    if (gpuList)
      ++completedComponents;
    if (mbInfo)
      ++completedComponents;
    if (cpuList)
      ++completedComponents;
    if (audioList)
      ++completedComponents;
    if (monitorList)
      ++completedComponents;
    return static_cast<double>(completedComponents) / totalComponents;
  }

  void Reset() noexcept {
    gpuList.reset();
    mbInfo.reset();
    cpuList.reset();
    audioList.reset();
    monitorList.reset();
  }

  void ResetGPUInfo() noexcept { gpuList.reset(); }

  void ResetMotherboardInfo() noexcept { mbInfo.reset(); }

  void ResetCPUInfo() noexcept { cpuList.reset(); }

  void ResetAudioInfo() noexcept { audioList.reset(); }

  void ResetMonitorInfo() noexcept { monitorList.reset(); }
};

struct DynamicInfo {
  std::unique_ptr<MemoryInfo> memInfo;
  std::unique_ptr<StorageList> storageList;
  std::unique_ptr<BatteryInfo> batteryInfo;
  std::unique_ptr<NetworkList> networkList;

  DynamicInfo() = default;

  DynamicInfo(DynamicInfo &&) = default;
  DynamicInfo &operator=(DynamicInfo &&) = default;

  DynamicInfo(const DynamicInfo &) = delete;
  DynamicInfo &operator=(const DynamicInfo &) = delete;

  [[nodiscard]] bool IsComplete() const noexcept { return memInfo && storageList && batteryInfo && networkList; }

  [[nodiscard]] bool IsEssentialComplete() const noexcept { return memInfo && storageList && networkList; }

  [[nodiscard]] bool HasMemoryInfo() const noexcept { return static_cast<bool>(memInfo); }

  [[nodiscard]] bool HasStorageInfo() const noexcept { return static_cast<bool>(storageList); }

  [[nodiscard]] bool HasBatteryInfo() const noexcept { return static_cast<bool>(batteryInfo); }

  [[nodiscard]] bool HasNetworkInfo() const noexcept { return static_cast<bool>(networkList); }

  [[nodiscard]] double GetCompletionPercentage() const noexcept {
    constexpr int totalComponents = 4;
    int completedComponents = 0;
    if (memInfo)
      ++completedComponents;
    if (storageList)
      ++completedComponents;
    if (batteryInfo)
      ++completedComponents;
    if (networkList)
      ++completedComponents;
    return static_cast<double>(completedComponents) / totalComponents;
  }

  void Reset() noexcept {
    memInfo.reset();
    storageList.reset();
    batteryInfo.reset();
    networkList.reset();
  }

  void ResetMemoryInfo() noexcept { memInfo.reset(); }

  void ResetStorageInfo() noexcept { storageList.reset(); }

  void ResetBatteryInfo() noexcept { batteryInfo.reset(); }

  void ResetNetworkInfo() noexcept { networkList.reset(); }
};

}  // namespace nysys

#endif