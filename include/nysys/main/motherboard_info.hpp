#ifndef MOTHERBOARD_INFO_HPP
#define MOTHERBOARD_INFO_HPP

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

namespace nysys {

enum class MotherboardError {
  Success = 0,
  WMISessionFailed,
  QueryExecutionFailed,
  PropertyRetrievalFailed,
  InvalidParameter
};

[[nodiscard]] constexpr std::string_view ToString(MotherboardError error) noexcept {
  switch (error) {
    case MotherboardError::Success:
      return "Success";
    case MotherboardError::WMISessionFailed:
      return "WMI session initialization failed";
    case MotherboardError::QueryExecutionFailed:
      return "WMI query execution failed";
    case MotherboardError::PropertyRetrievalFailed:
      return "Motherboard property retrieval failed";
    case MotherboardError::InvalidParameter:
      return "Invalid parameter";
    default:
      return "Unknown error";
  }
}

template <typename T>
using MotherboardResult = std::optional<T>;

class MotherboardInfo {
public:
  MotherboardInfo() noexcept;

  ~MotherboardInfo() = default;

  MotherboardInfo(MotherboardInfo &&other) noexcept = default;
  MotherboardInfo &operator=(MotherboardInfo &&other) noexcept = default;

  MotherboardInfo(const MotherboardInfo &) = delete;
  MotherboardInfo &operator=(const MotherboardInfo &) = delete;

  [[nodiscard]] const std::string &GetProduct() const noexcept;
  [[nodiscard]] const std::string &GetManufacturer() const noexcept;
  [[nodiscard]] const std::string &GetSerial() const noexcept;
  [[nodiscard]] const std::string &GetBiosVersion() const noexcept;
  [[nodiscard]] const std::string &GetBiosSerial() const noexcept;
  [[nodiscard]] const std::string &GetSystemSKU() const noexcept;
  [[nodiscard]] bool IsInitialized() const noexcept;
  [[nodiscard]] MotherboardError GetLastError() const noexcept;

private:
  std::string m_productName;
  std::string m_manufacturer;
  std::string m_serialNumber;
  std::string m_biosVersion;
  std::string m_biosSerial;
  std::string m_systemSKU;
  bool m_initialized = false;
  MotherboardError m_lastError = MotherboardError::Success;

  void Initialize() noexcept;
};

[[nodiscard]] std::unique_ptr<MotherboardInfo> GetMotherboardInfo();

namespace detail {

constexpr std::string_view kUnknownMotherboardProduct = "Unknown";
constexpr std::string_view kUnknownMotherboardManufacturer = "Unknown";
constexpr std::string_view kUnknownMotherboardSerial = "Unknown";
constexpr std::string_view kUnknownMotherboardBiosVersion = "Unknown";
constexpr std::string_view kUnknownMotherboardBiosSerial = "Unknown";
constexpr std::string_view kUnknownMotherboardSystemSKU = "Unknown";
}  // namespace detail

}  // namespace nysys

#endif