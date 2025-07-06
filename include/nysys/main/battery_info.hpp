#ifndef BATTERY_INFO_HPP
#define BATTERY_INFO_HPP

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
#include <string_view>

namespace nysys {

enum class BatteryError { Success = 0, SystemPowerStatusFailed, InvalidBatteryState, InvalidParameter };

[[nodiscard]] constexpr std::string_view ToString(BatteryError error) noexcept {
  switch (error) {
    case BatteryError::Success:
      return "Success";
    case BatteryError::SystemPowerStatusFailed:
      return "Failed to retrieve system power status";
    case BatteryError::InvalidBatteryState:
      return "Invalid battery state detected";
    case BatteryError::InvalidParameter:
      return "Invalid parameter";
    default:
      return "Unknown error";
  }
}

template <typename T>
using BatteryResult = std::optional<T>;

class BatteryInfo {
public:
  BatteryInfo() noexcept;

  ~BatteryInfo() = default;

  BatteryInfo(BatteryInfo &&other) noexcept = default;
  BatteryInfo &operator=(BatteryInfo &&other) noexcept = default;

  BatteryInfo(const BatteryInfo &) = delete;
  BatteryInfo &operator=(const BatteryInfo &) = delete;

  [[nodiscard]] uint8_t GetPercent() const noexcept;
  [[nodiscard]] bool IsPluggedIn() const noexcept;
  [[nodiscard]] bool IsDesktop() const noexcept;
  [[nodiscard]] bool IsInitialized() const noexcept;
  [[nodiscard]] BatteryError GetLastError() const noexcept;

private:
  uint8_t m_percent = 100;
  bool m_pluggedIn = true;
  bool m_isDesktop = true;
  bool m_initialized = false;
  BatteryError m_lastError = BatteryError::Success;

  void Initialize() noexcept;
};

[[nodiscard]] std::unique_ptr<BatteryInfo> GetBatteryInfo();

namespace detail {

constexpr uint8_t kBatteryStatusUnknown = 255;
constexpr uint8_t kBatteryFlagNoBattery = 128;
constexpr uint8_t kBatteryFlagUnknown = 255;
constexpr uint8_t kACLineStatusOnline = 1;
constexpr uint8_t kDefaultDesktopBatteryPercent = 100;
}  // namespace detail

}  // namespace nysys

#endif