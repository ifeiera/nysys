#include "main/monitor_info.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <utility>

namespace nysys {
namespace detail {

// https://gist.github.com/texus/3212ebc1ed1502ecd265cc7cf1322b02
// code referance


[[nodiscard]] std::string SafeStringConversion(std::string_view input, std::string_view fallback) noexcept {
  try {
    return std::string{input};
  } catch (...) {
    return std::string{fallback};
  }
}

template <typename T>
[[nodiscard]] T SafeNumericConversion(T value, T minValue, T maxValue, T fallback) noexcept {
  if (value >= minValue && value <= maxValue) {
    return value;
  }
  return fallback;
}

}  // namespace detail

static const GUID GUID_DEVINTERFACE_MONITOR = {
    0xe6f07b5f, 0xee97, 0x4a90, {0xb0, 0x76, 0x33, 0xf5, 0x7b, 0xf4, 0xea, 0xa7}};

int MonitorInfo::GetWidth() const noexcept { return m_width; }

int MonitorInfo::GetHeight() const noexcept { return m_height; }

bool MonitorInfo::IsPrimary() const noexcept { return m_isPrimary; }

std::string MonitorInfo::GetDeviceId() const {
  std::string escapedId = m_deviceId;
  size_t pos = 0;
  while ((pos = escapedId.find('\\', pos)) != std::string::npos) {
    escapedId.replace(pos, 1, "\\\\");
    pos += 2;
  }
  return escapedId;
}

const std::string &MonitorInfo::GetManufacturer() const noexcept { return m_manufacturer; }

const std::string &MonitorInfo::GetAspectRatio() const noexcept { return m_aspectRatio; }

const std::string &MonitorInfo::GetNativeResolution() const noexcept { return m_nativeResolution; }

int MonitorInfo::GetRefreshRate() const noexcept { return m_refreshRate; }

const std::string &MonitorInfo::GetCurrentResolution() const noexcept { return m_currentResolution; }

int MonitorInfo::GetPhysicalWidthMm() const noexcept { return m_physicalWidthMm; }

int MonitorInfo::GetPhysicalHeightMm() const noexcept { return m_physicalHeightMm; }

const std::string &MonitorInfo::GetScreenSize() const noexcept { return m_screenSize; }

MonitorList::MonitorList() noexcept { Initialize(); }

void MonitorList::Initialize() noexcept {
  try {
    m_monitors.clear();
    m_lastError = MonitorError::Success;
    m_initialized = false;

    BOOL result = EnumDisplayMonitors(NULL, NULL, detail::MonitorEnumProc, reinterpret_cast<LPARAM>(this));

    if (!result) {
      m_lastError = MonitorError::EnumerationFailed;
      return;
    }

    m_initialized = true;
  } catch (...) {
    m_lastError = MonitorError::EnumerationFailed;
    m_initialized = false;
  }
}

size_t MonitorList::GetCount() const noexcept { return m_monitors.size(); }

const MonitorInfo *MonitorList::GetMonitor(size_t index) const noexcept {
  if (index < m_monitors.size()) {
    return &m_monitors[index];
  }
  return nullptr;
}

const std::vector<MonitorInfo> &MonitorList::GetMonitors() const noexcept { return m_monitors; }

bool MonitorList::IsInitialized() const noexcept { return m_initialized; }

MonitorError MonitorList::GetLastError() const noexcept { return m_lastError; }

void MonitorList::AddMonitor(const MonitorInfo &monitor) noexcept {
  try {
    m_monitors.push_back(monitor);
  } catch (...) {
  }
}

std::unique_ptr<MonitorList> GetMonitorList() { return std::make_unique<MonitorList>(); }

namespace detail {

double CalculatePPI(int width, int height, double diagonalInch) noexcept {
  if (diagonalInch <= 0.0 || width <= 0 || height <= 0) {
    return 0.0;
  }

  try {
    double diagonalPixels = std::sqrt(static_cast<double>(width * width + height * height));
    return diagonalPixels / diagonalInch;
  } catch (...) {
    return 0.0;
  }
}

bool GetMonitorSizeFromEDID(std::string_view deviceName, int *widthMm, int *heightMm) noexcept {
  if (!widthMm || !heightMm || deviceName.empty()) {
    return false;
  }

  *widthMm = 0;
  *heightMm = 0;

  try {
    HDEVINFO hDevInfo =
        SetupDiGetClassDevs(&GUID_DEVINTERFACE_MONITOR, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
      return false;
    }

    struct DevInfoSetDeleter {
      void operator()(HDEVINFO handle) {
        if (handle != INVALID_HANDLE_VALUE) {
          SetupDiDestroyDeviceInfoList(handle);
        }
      }
    };

    std::unique_ptr<void, DevInfoSetDeleter> devInfoGuard(hDevInfo);

    SP_DEVICE_INTERFACE_DATA devInfo = {0};
    devInfo.cbSize = sizeof(devInfo);

    DWORD monitorIndex = 0;
    while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_MONITOR, monitorIndex, &devInfo)) {
      monitorIndex++;

      DWORD requiredSize = 0;
      SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInfo, NULL, 0, &requiredSize, NULL);

      auto pDevDetail = std::make_unique<char[]>(requiredSize);
      auto pDetail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(pDevDetail.get());
      pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

      SP_DEVINFO_DATA devInfoData = {0};
      devInfoData.cbSize = sizeof(devInfoData);

      if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInfo, pDetail, requiredSize, NULL, &devInfoData)) {
        continue;
      }

      HKEY hEDIDRegKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
      if (hEDIDRegKey && hEDIDRegKey != INVALID_HANDLE_VALUE) {
        BYTE edid[1024];
        DWORD edidSize = sizeof(edid);

        if (RegQueryValueExA(hEDIDRegKey, "EDID", NULL, NULL, edid, &edidSize) == ERROR_SUCCESS) {
          *widthMm = ((edid[68] & 0xF0) << 4) + edid[66];
          *heightMm = ((edid[68] & 0x0F) << 8) + edid[67];

          RegCloseKey(hEDIDRegKey);
          return true;
        }
        RegCloseKey(hEDIDRegKey);
      }
    }

    return false;
  } catch (...) {
    return false;
  }
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) {
  auto list = reinterpret_cast<MonitorList *>(dwData);
  if (!list) {
    return FALSE;
  }

  MonitorInfo monitor;

  try {
    MONITORINFOEXA monitorInfo = {0};
    monitorInfo.cbSize = sizeof(MONITORINFOEXA);

    if (!GetMonitorInfoA(hMonitor, reinterpret_cast<LPMONITORINFO>(&monitorInfo))) {
      return TRUE;
    }

    int width = SafeNumericConversion(static_cast<int>(monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left), 1,
                                      32767, 1920);
    int height = SafeNumericConversion(static_cast<int>(monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top), 1,
                                       32767, 1080);
    bool isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;

    MonitorInfoAccess::SetWidth(monitor, width);
    MonitorInfoAccess::SetHeight(monitor, height);
    MonitorInfoAccess::SetIsPrimary(monitor, isPrimary);

    DISPLAY_DEVICEA displayDevice = {0};
    displayDevice.cb = sizeof(displayDevice);

    if (EnumDisplayDevicesA(monitorInfo.szDevice, 0, &displayDevice, 0)) {
      MonitorInfoAccess::SetDeviceId(monitor, SafeStringConversion(displayDevice.DeviceID, "Unknown"));

      try {
        std::string deviceId = displayDevice.DeviceID;
        auto start = deviceId.find('\\');
        if (start != std::string::npos) {
          auto end = deviceId.find('\\', start + 1);
          if (end != std::string::npos) {
            std::string manufacturer = deviceId.substr(start + 1, end - start - 1);
            MonitorInfoAccess::SetManufacturer(monitor, std::move(manufacturer));
          } else {
            MonitorInfoAccess::SetManufacturer(monitor, std::string{kUnknownManufacturer});
          }
        } else {
          MonitorInfoAccess::SetManufacturer(monitor, std::string{kUnknownManufacturer});
        }
      } catch (...) {
        MonitorInfoAccess::SetManufacturer(monitor, std::string{kUnknownManufacturer});
      }
    } else {
      MonitorInfoAccess::SetDeviceId(monitor, "Unknown");
      MonitorInfoAccess::SetManufacturer(monitor, std::string{kUnknownManufacturer});
    }

    DEVMODEA dm = {0};
    dm.dmSize = sizeof(dm);
    int refreshRate = 60;

    if (EnumDisplaySettingsA(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &dm)) {
      refreshRate = SafeNumericConversion(static_cast<int>(dm.dmDisplayFrequency), 1, 1000, 60);
      MonitorInfoAccess::SetRefreshRate(monitor, refreshRate);

      try {
        uint32_t dmWidth = dm.dmPelsWidth;
        uint32_t dmHeight = dm.dmPelsHeight;

        if (dmWidth > 0 && dmHeight > 0) {
          uint32_t gcd = std::gcd(dmWidth, dmHeight);
          if (gcd > 0) {
            std::string aspectRatio = std::to_string(dmWidth / gcd) + ":" + std::to_string(dmHeight / gcd);
            MonitorInfoAccess::SetAspectRatio(monitor, std::move(aspectRatio));
          } else {
            MonitorInfoAccess::SetAspectRatio(monitor, std::string{kDefaultAspectRatio});
          }

          std::string resolution = std::to_string(dmWidth) + " x " + std::to_string(dmHeight);
          MonitorInfoAccess::SetNativeResolution(monitor, resolution);

          std::string currentRes = resolution + " @ " + std::to_string(refreshRate) + " Hz";
          MonitorInfoAccess::SetCurrentResolution(monitor, std::move(currentRes));
        } else {
          MonitorInfoAccess::SetAspectRatio(monitor, std::string{kDefaultAspectRatio});
          MonitorInfoAccess::SetNativeResolution(monitor, std::string{kDefaultResolution});
          MonitorInfoAccess::SetCurrentResolution(monitor, std::string{kDefaultResolution});
        }
      } catch (...) {
        MonitorInfoAccess::SetAspectRatio(monitor, std::string{kDefaultAspectRatio});
        MonitorInfoAccess::SetNativeResolution(monitor, std::string{kDefaultResolution});
        MonitorInfoAccess::SetCurrentResolution(monitor, std::string{kDefaultResolution});
      }
    } else {
      MonitorInfoAccess::SetRefreshRate(monitor, refreshRate);
      MonitorInfoAccess::SetAspectRatio(monitor, std::string{kDefaultAspectRatio});
      MonitorInfoAccess::SetNativeResolution(monitor, std::string{kDefaultResolution});
      MonitorInfoAccess::SetCurrentResolution(monitor, std::string{kDefaultResolution});
    }

    int physicalWidthMm = 0, physicalHeightMm = 0;
    bool gotEDID = false;

    try {
      gotEDID = GetMonitorSizeFromEDID(displayDevice.DeviceID, &physicalWidthMm, &physicalHeightMm);
    } catch (...) {
      gotEDID = false;
    }

    if (gotEDID && physicalWidthMm > 0 && physicalHeightMm > 0) {
      MonitorInfoAccess::SetPhysicalWidthMm(monitor, physicalWidthMm);
      MonitorInfoAccess::SetPhysicalHeightMm(monitor, physicalHeightMm);

      try {
        double diagonal_mm =
            std::sqrt(static_cast<double>(physicalWidthMm * physicalWidthMm + physicalHeightMm * physicalHeightMm));
        double diagonal_inch = diagonal_mm / 25.4;

        if (diagonal_inch > 0.0 && diagonal_inch < 1000.0) {
          std::ostringstream oss;
          oss << std::fixed << std::setprecision(1) << diagonal_inch << " inch";
          MonitorInfoAccess::SetScreenSize(monitor, oss.str());
        } else {
          MonitorInfoAccess::SetScreenSize(monitor, std::string{kDefaultScreenSize});
        }
      } catch (...) {
        MonitorInfoAccess::SetScreenSize(monitor, std::string{kDefaultScreenSize});
      }
    } else {
      MonitorInfoAccess::SetPhysicalWidthMm(monitor, 0);
      MonitorInfoAccess::SetPhysicalHeightMm(monitor, 0);
      MonitorInfoAccess::SetScreenSize(monitor, std::string{kDefaultScreenSize});
    }
  } catch (...) {
    MonitorInfoAccess::SetWidth(monitor, 1920);
    MonitorInfoAccess::SetHeight(monitor, 1080);
    MonitorInfoAccess::SetIsPrimary(monitor, false);
    MonitorInfoAccess::SetDeviceId(monitor, "Unknown");
    MonitorInfoAccess::SetManufacturer(monitor, std::string{kUnknownManufacturer});
    MonitorInfoAccess::SetAspectRatio(monitor, std::string{kDefaultAspectRatio});
    MonitorInfoAccess::SetNativeResolution(monitor, std::string{kDefaultResolution});
    MonitorInfoAccess::SetRefreshRate(monitor, 60);
    MonitorInfoAccess::SetCurrentResolution(monitor, std::string{kDefaultResolution});
    MonitorInfoAccess::SetPhysicalWidthMm(monitor, 0);
    MonitorInfoAccess::SetPhysicalHeightMm(monitor, 0);
    MonitorInfoAccess::SetScreenSize(monitor, std::string{kDefaultScreenSize});
  }

  list->AddMonitor(monitor);

  return TRUE;
}

}  // namespace detail

}  // namespace nysys