#ifndef MEMORY_INFO_HPP
#define MEMORY_INFO_HPP

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

enum class MemoryError {
  Success = 0,
  WMISessionFailed,
  QueryExecutionFailed,
  PropertyRetrievalFailed,
  InvalidParameter
};

[[nodiscard]] constexpr std::string_view ToString(MemoryError error) noexcept {
  switch (error) {
    case MemoryError::Success:
      return "Success";
    case MemoryError::WMISessionFailed:
      return "WMI session initialization failed";
    case MemoryError::QueryExecutionFailed:
      return "WMI query execution failed";
    case MemoryError::PropertyRetrievalFailed:
      return "Memory property retrieval failed";
    case MemoryError::InvalidParameter:
      return "Invalid parameter";
    default:
      return "Unknown error";
  }
}

template <typename T>
using MemoryResult = std::optional<T>;

class RAMSlotInfo {
public:
  RAMSlotInfo() = default;

  RAMSlotInfo(uint64_t ramCapacity, uint32_t ramSpeed, uint32_t ramConfigSpeed, std::string slotName,
              std::string mfr) noexcept;

  [[nodiscard]] const std::string &GetSlotLocation() const noexcept;
  [[nodiscard]] const std::string &GetManufacturer() const noexcept;
  [[nodiscard]] uint64_t GetCapacity() const noexcept;
  [[nodiscard]] uint32_t GetSpeed() const noexcept;
  [[nodiscard]] uint32_t GetConfiguredSpeed() const noexcept;

private:
  uint64_t m_capacity = 0;
  uint32_t m_speed = 0;
  uint32_t m_configuredSpeed = 0;
  std::string m_slot;
  std::string m_manufacturer;
};

class MemoryInfo {
public:
  MemoryInfo() noexcept;

  ~MemoryInfo() = default;

  MemoryInfo(MemoryInfo &&other) noexcept = default;
  MemoryInfo &operator=(MemoryInfo &&other) noexcept = default;

  MemoryInfo(const MemoryInfo &) = delete;
  MemoryInfo &operator=(const MemoryInfo &) = delete;

  [[nodiscard]] uint64_t GetTotalPhysical() const noexcept;
  [[nodiscard]] uint64_t GetAvailablePhysical() const noexcept;
  [[nodiscard]] uint64_t GetUsedPhysical() const noexcept;
  [[nodiscard]] uint32_t GetMemoryLoad() const noexcept;
  [[nodiscard]] size_t GetRAMSlotCount() const noexcept;
  [[nodiscard]] const RAMSlotInfo *GetRAMSlot(size_t index) const noexcept;
  [[nodiscard]] const std::vector<RAMSlotInfo> &GetRAMSlots() const noexcept;
  [[nodiscard]] bool IsInitialized() const noexcept;
  [[nodiscard]] MemoryError GetLastError() const noexcept;

private:
  uint64_t m_totalPhys = 0;
  uint64_t m_availPhys = 0;
  uint64_t m_usedPhys = 0;
  uint32_t m_memoryLoad = 0;
  std::vector<RAMSlotInfo> m_ramSlots;
  bool m_initialized = false;
  MemoryError m_lastError = MemoryError::Success;

  void Initialize() noexcept;
};

[[nodiscard]] std::unique_ptr<MemoryInfo> GetMemoryInfo();

namespace detail {

constexpr std::string_view kUnknownRamSlot = "Unknown Slot";
constexpr std::string_view kUnknownRamManufacturer = "Unknown Manufacturer";
}  // namespace detail

}  // namespace nysys

#endif