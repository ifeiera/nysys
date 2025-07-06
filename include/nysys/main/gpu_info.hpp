#ifndef GPU_INFO_HPP
#define GPU_INFO_HPP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <cstdint>
#include <dxgi.h>
#include <initguid.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <wrl/client.h>

namespace nysys {

enum class GPUError {
  Success = 0,
  DXGIFactoryCreationFailed,
  AdapterEnumerationFailed,
  AdapterDescriptionFailed,
  InvalidParameter
};

[[nodiscard]] constexpr std::string_view ToString(GPUError error) noexcept {
  switch (error) {
    case GPUError::Success:
      return "Success";
    case GPUError::DXGIFactoryCreationFailed:
      return "DXGI factory creation failed";
    case GPUError::AdapterEnumerationFailed:
      return "Adapter enumeration failed";
    case GPUError::AdapterDescriptionFailed:
      return "Adapter description retrieval failed";
    case GPUError::InvalidParameter:
      return "Invalid parameter";
    default:
      return "Unknown error";
  }
}

template <typename T>
using GPUResult = std::optional<T>;

class GPUInfo {
public:
  GPUInfo() = default;

  GPUInfo(std::string gpuName, double dedicatedMem, double sharedMem, bool integrated, uint32_t index) noexcept;

  [[nodiscard]] const std::string &GetName() const noexcept;
  [[nodiscard]] double GetDedicatedMemory() const noexcept;
  [[nodiscard]] double GetSharedMemory() const noexcept;
  [[nodiscard]] bool IsIntegrated() const noexcept;
  [[nodiscard]] uint32_t GetAdapterIndex() const noexcept;

private:
  std::string m_name;
  double m_dedicatedMemory = 0.0;
  double m_sharedMemory = 0.0;
  bool m_isIntegrated = false;
  uint32_t m_adapterIndex = 0;
};

class GPUList {
public:
  GPUList() noexcept;

  ~GPUList() = default;

  GPUList(GPUList &&other) noexcept = default;
  GPUList &operator=(GPUList &&other) noexcept = default;

  GPUList(const GPUList &) = delete;
  GPUList &operator=(const GPUList &) = delete;

  [[nodiscard]] size_t GetCount() const noexcept;
  [[nodiscard]] const GPUInfo *GetGPU(size_t index) const noexcept;
  [[nodiscard]] const std::vector<GPUInfo> &GetGPUs() const noexcept;
  [[nodiscard]] bool IsInitialized() const noexcept;
  [[nodiscard]] GPUError GetLastError() const noexcept;

private:
  std::vector<GPUInfo> m_gpus;
  bool m_initialized = false;
  GPUError m_lastError = GPUError::Success;

  void Initialize() noexcept;
};

[[nodiscard]] std::unique_ptr<GPUList> GetGPUList();

namespace detail {

[[nodiscard]] std::string WideStringToUtf8(const wchar_t *wstr) noexcept;

constexpr uint64_t kIntegratedGpuMemoryThreshold = 512ULL * 1024 * 1024;
}  // namespace detail

}  // namespace nysys

#endif