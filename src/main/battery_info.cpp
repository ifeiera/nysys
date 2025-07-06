#include "main/battery_info.hpp"

namespace nysys {
namespace detail {

[[nodiscard]] bool HasBattery(const SYSTEM_POWER_STATUS &powerStatus) noexcept {
  if (powerStatus.BatteryFlag == kBatteryFlagNoBattery || powerStatus.BatteryFlag == kBatteryFlagUnknown) {
    return false;
  }

  if (powerStatus.BatteryLifePercent == kBatteryStatusUnknown && powerStatus.BatteryFlag == 1) {
    return false;
  }

  return powerStatus.BatteryLifePercent != kBatteryStatusUnknown;
}

[[nodiscard]] uint8_t SafeBatteryPercent(uint8_t rawPercent) noexcept {
  if (rawPercent == kBatteryStatusUnknown) {
    return kDefaultDesktopBatteryPercent;
  }
  return rawPercent <= 100 ? rawPercent : kDefaultDesktopBatteryPercent;
}
}  // namespace detail

BatteryInfo::BatteryInfo() noexcept { Initialize(); }

void BatteryInfo::Initialize() noexcept {
  try {
    SYSTEM_POWER_STATUS powerStatus;
    if (!GetSystemPowerStatus(&powerStatus)) {
      m_lastError = BatteryError::SystemPowerStatusFailed;
      return;
    }

    const bool hasBattery = detail::HasBattery(powerStatus);
    m_isDesktop = !hasBattery;

    if (hasBattery) {
      m_percent = detail::SafeBatteryPercent(powerStatus.BatteryLifePercent);
      m_pluggedIn = (powerStatus.ACLineStatus == detail::kACLineStatusOnline);
    } else {
      m_percent = detail::kDefaultDesktopBatteryPercent;
      m_pluggedIn = true;
    }

    m_initialized = true;
    m_lastError = BatteryError::Success;
  } catch (...) {
    m_lastError = BatteryError::InvalidBatteryState;
  }
}

uint8_t BatteryInfo::GetPercent() const noexcept { return m_percent; }

bool BatteryInfo::IsPluggedIn() const noexcept { return m_pluggedIn; }

bool BatteryInfo::IsDesktop() const noexcept { return m_isDesktop; }

bool BatteryInfo::IsInitialized() const noexcept { return m_initialized; }

BatteryError BatteryInfo::GetLastError() const noexcept { return m_lastError; }

std::unique_ptr<BatteryInfo> GetBatteryInfo() { return std::make_unique<BatteryInfo>(); }

}  // namespace nysys