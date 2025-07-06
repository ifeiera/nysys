#ifndef MONITOR_INFO_HPP
#define MONITOR_INFO_HPP

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
#include <setupapi.h>
#include <string>
#include <string_view>
#include <vector>

namespace nysys {

enum class MonitorError {
  Success = 0,
  EnumerationFailed,
  DeviceInfoFailed,
  RegistryAccessFailed,
  EDIDRetrievalFailed,
  InvalidParameter
};

[[nodiscard]] constexpr std::string_view ToString(MonitorError error) noexcept {
  switch (error) {
    case MonitorError::Success:
      return "Success";
    case MonitorError::EnumerationFailed:
      return "Monitor enumeration failed";
    case MonitorError::DeviceInfoFailed:
      return "Device information retrieval failed";
    case MonitorError::RegistryAccessFailed:
      return "Registry access failed";
    case MonitorError::EDIDRetrievalFailed:
      return "EDID data retrieval failed";
    case MonitorError::InvalidParameter:
      return "Invalid parameter";
    default:
      return "Unknown error";
  }
}

template <typename T>
using MonitorResult = std::optional<T>;

namespace detail {

class MonitorInfoAccess;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
}  // namespace detail

class MonitorInfo {
public:
  MonitorInfo() = default;

  [[nodiscard]] int GetWidth() const noexcept;
  [[nodiscard]] int GetHeight() const noexcept;
  [[nodiscard]] bool IsPrimary() const noexcept;
  [[nodiscard]] std::string GetDeviceId() const;
  [[nodiscard]] const std::string &GetManufacturer() const noexcept;
  [[nodiscard]] const std::string &GetAspectRatio() const noexcept;
  [[nodiscard]] const std::string &GetNativeResolution() const noexcept;
  [[nodiscard]] int GetRefreshRate() const noexcept;
  [[nodiscard]] const std::string &GetCurrentResolution() const noexcept;
  [[nodiscard]] int GetPhysicalWidthMm() const noexcept;
  [[nodiscard]] int GetPhysicalHeightMm() const noexcept;
  [[nodiscard]] const std::string &GetScreenSize() const noexcept;

  friend class MonitorList;
  friend class detail::MonitorInfoAccess;

  friend BOOL CALLBACK detail::MonitorEnumProc(HMONITOR, HDC, LPRECT, LPARAM);

private:
  int m_width = 0;
  int m_height = 0;
  bool m_isPrimary = false;
  std::string m_deviceId;
  std::string m_manufacturer;
  std::string m_aspectRatio;
  std::string m_nativeResolution;
  int m_refreshRate = 0;
  std::string m_currentResolution;
  int m_physicalWidthMm = 0;
  int m_physicalHeightMm = 0;
  std::string m_screenSize;
};

class MonitorList {
public:
  MonitorList() noexcept;

  ~MonitorList() = default;

  MonitorList(MonitorList &&other) noexcept = default;
  MonitorList &operator=(MonitorList &&other) noexcept = default;

  MonitorList(const MonitorList &) = delete;
  MonitorList &operator=(const MonitorList &) = delete;

  [[nodiscard]] size_t GetCount() const noexcept;
  [[nodiscard]] const MonitorInfo *GetMonitor(size_t index) const noexcept;
  [[nodiscard]] const std::vector<MonitorInfo> &GetMonitors() const noexcept;
  [[nodiscard]] bool IsInitialized() const noexcept;
  [[nodiscard]] MonitorError GetLastError() const noexcept;

  void AddMonitor(const MonitorInfo &monitor) noexcept;

private:
  std::vector<MonitorInfo> m_monitors;
  bool m_initialized = false;
  MonitorError m_lastError = MonitorError::Success;

  void Initialize() noexcept;
};

[[nodiscard]] std::unique_ptr<MonitorList> GetMonitorList();

namespace detail {

class MonitorInfoAccess {
public:
  static void SetWidth(MonitorInfo &m, int w) noexcept { m.m_width = w; }

  static void SetHeight(MonitorInfo &m, int h) noexcept { m.m_height = h; }

  static void SetIsPrimary(MonitorInfo &m, bool p) noexcept { m.m_isPrimary = p; }

  static void SetDeviceId(MonitorInfo &m, std::string id) noexcept { m.m_deviceId = std::move(id); }

  static void SetManufacturer(MonitorInfo &m, std::string mfr) noexcept { m.m_manufacturer = std::move(mfr); }

  static void SetAspectRatio(MonitorInfo &m, std::string ar) noexcept { m.m_aspectRatio = std::move(ar); }

  static void SetNativeResolution(MonitorInfo &m, std::string res) noexcept { m.m_nativeResolution = std::move(res); }

  static void SetRefreshRate(MonitorInfo &m, int rate) noexcept { m.m_refreshRate = rate; }

  static void SetCurrentResolution(MonitorInfo &m, std::string res) noexcept { m.m_currentResolution = std::move(res); }

  static void SetPhysicalWidthMm(MonitorInfo &m, int w) noexcept { m.m_physicalWidthMm = w; }

  static void SetPhysicalHeightMm(MonitorInfo &m, int h) noexcept { m.m_physicalHeightMm = h; }

  static void SetScreenSize(MonitorInfo &m, std::string size) noexcept { m.m_screenSize = std::move(size); }
};

[[nodiscard]] double CalculatePPI(int width, int height, double diagonalInch) noexcept;
[[nodiscard]] bool GetMonitorSizeFromEDID(std::string_view deviceName, int *widthMm, int *heightMm) noexcept;

constexpr std::string_view kUnknownManufacturer = "Unknown";
constexpr std::string_view kDefaultAspectRatio = "0:0";
constexpr std::string_view kDefaultResolution = "Unknown";
constexpr std::string_view kDefaultScreenSize = "Unknown";
}  // namespace detail

}  // namespace nysys

#endif