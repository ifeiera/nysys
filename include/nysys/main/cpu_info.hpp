#ifndef CPU_INFO_HPP
#define CPU_INFO_HPP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nysys {

enum class CPUError { Success = 0, WMISessionFailed, QueryExecutionFailed, PropertyRetrievalFailed, InvalidParameter };

[[nodiscard]] constexpr std::string_view ToString(CPUError error) noexcept {
  switch (error) {
    case CPUError::Success:
      return "Success";
    case CPUError::WMISessionFailed:
      return "WMI session initialization failed";
    case CPUError::QueryExecutionFailed:
      return "WMI query execution failed";
    case CPUError::PropertyRetrievalFailed:
      return "CPU property retrieval failed";
    case CPUError::InvalidParameter:
      return "Invalid parameter";
    default:
      return "Unknown error";
  }
}

template <typename T>
using CPUResult = std::optional<T>;

class CPUInfo {
public:
  CPUInfo() = default;

  CPUInfo(std::string cpuName, uint32_t cpuCores, uint32_t cpuThreads, uint32_t cpuClockSpeed) noexcept;

  [[nodiscard]] const std::string &GetName() const noexcept;
  [[nodiscard]] uint32_t GetCores() const noexcept;
  [[nodiscard]] uint32_t GetThreads() const noexcept;
  [[nodiscard]] uint32_t GetClockSpeed() const noexcept;

private:
  std::string m_name;
  uint32_t m_cores = 0;
  uint32_t m_threads = 0;
  uint32_t m_clockSpeed = 0;
};

class CPUList {
public:
  CPUList() noexcept;

  ~CPUList() = default;

  CPUList(CPUList &&other) noexcept = default;
  CPUList &operator=(CPUList &&other) noexcept = default;

  CPUList(const CPUList &) = delete;
  CPUList &operator=(const CPUList &) = delete;

  [[nodiscard]] size_t GetCount() const noexcept;
  [[nodiscard]] const CPUInfo *GetCPU(size_t index) const noexcept;
  [[nodiscard]] const std::vector<CPUInfo> &GetCPUs() const noexcept;
  [[nodiscard]] bool IsInitialized() const noexcept;
  [[nodiscard]] CPUError GetLastError() const noexcept;

private:
  std::vector<CPUInfo> m_cpus;
  bool m_initialized = false;
  CPUError m_lastError = CPUError::Success;

  void Initialize() noexcept;
};

[[nodiscard]] std::unique_ptr<CPUList> GetCPUList();

namespace detail {

constexpr std::string_view kUnknownCpuName = "Unknown CPU";
}

}  // namespace nysys

#endif