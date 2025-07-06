#include "main/memory_info.hpp"

#include <algorithm>

#include "helper/wmi_helper.hpp"

namespace nysys {
namespace detail {

[[nodiscard]] std::string GetSafeMemoryProperty(IWbemClassObject *pclsObj, std::wstring_view property,
                                                std::string_view fallback) noexcept {
  if (!pclsObj) {
    return std::string{fallback};
  }

  std::string result = wmi::GetPropertyString(pclsObj, property);
  return result.empty() ? std::string{fallback} : result;
}

[[nodiscard]] uint64_t GetSafeUint64Property(IWbemClassObject *pclsObj, std::wstring_view property) noexcept {
  if (!pclsObj) {
    return 0;
  }

  VARIANT vtProp;
  VariantInit(&vtProp);

  uint64_t result = 0;
  if (SUCCEEDED(pclsObj->Get(property.data(), 0, &vtProp, 0, 0)) && vtProp.vt != VT_NULL) {
    if (vtProp.vt == VT_BSTR && vtProp.bstrVal != nullptr) {
      result = _wtoi64(vtProp.bstrVal);
    }
  }

  VariantClear(&vtProp);
  return result;
}

[[nodiscard]] uint32_t GetSafeUint32Property(IWbemClassObject *pclsObj, std::wstring_view property) noexcept {
  if (!pclsObj) {
    return 0;
  }

  VARIANT vtProp;
  VariantInit(&vtProp);

  uint32_t result = 0;
  if (SUCCEEDED(pclsObj->Get(property.data(), 0, &vtProp, 0, 0)) && vtProp.vt != VT_NULL) {
    result = vtProp.uintVal;
  }

  VariantClear(&vtProp);
  return result;
}
}  // namespace detail

RAMSlotInfo::RAMSlotInfo(uint64_t ramCapacity, uint32_t ramSpeed, uint32_t ramConfigSpeed, std::string slotName,
                         std::string mfr) noexcept
    : m_capacity(ramCapacity),
      m_speed(ramSpeed),
      m_configuredSpeed(ramConfigSpeed),
      m_slot(std::move(slotName)),
      m_manufacturer(std::move(mfr)) {}

const std::string &RAMSlotInfo::GetSlotLocation() const noexcept {
  if (m_slot.empty()) {
    static const std::string unknown{detail::kUnknownRamSlot};
    return unknown;
  }
  return m_slot;
}

const std::string &RAMSlotInfo::GetManufacturer() const noexcept {
  if (m_manufacturer.empty()) {
    static const std::string unknown{detail::kUnknownRamManufacturer};
    return unknown;
  }
  return m_manufacturer;
}

uint64_t RAMSlotInfo::GetCapacity() const noexcept { return m_capacity; }

uint32_t RAMSlotInfo::GetSpeed() const noexcept { return m_speed; }

uint32_t RAMSlotInfo::GetConfiguredSpeed() const noexcept { return m_configuredSpeed; }

MemoryInfo::MemoryInfo() noexcept { Initialize(); }

void MemoryInfo::Initialize() noexcept {
  try {
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&memStatus)) {
      m_totalPhys = memStatus.ullTotalPhys;
      m_availPhys = memStatus.ullAvailPhys;
      m_usedPhys = memStatus.ullTotalPhys - memStatus.ullAvailPhys;
      m_memoryLoad = memStatus.dwMemoryLoad;
    } else {
      m_lastError = MemoryError::PropertyRetrievalFailed;
      return;
    }

    wmi::WMISession wmiSession;
    if (!wmiSession.IsInitialized()) {
      m_lastError = MemoryError::WMISessionFailed;
      return;
    }

    auto pEnumerator = wmiSession.ExecuteQuery(
        L"SELECT Capacity, Speed, ConfiguredClockSpeed, DeviceLocator, "
        L"Manufacturer FROM Win32_PhysicalMemory");
    if (!pEnumerator) {
      m_lastError = MemoryError::QueryExecutionFailed;
      return;
    }

    IWbemClassObject *pclsObj = nullptr;
    ULONG uReturn = 0;

    while (SUCCEEDED(pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn)) && uReturn != 0) {
      try {
        uint64_t capacity = detail::GetSafeUint64Property(pclsObj, L"Capacity");
        uint32_t speed = detail::GetSafeUint32Property(pclsObj, L"Speed");
        uint32_t configuredSpeed = detail::GetSafeUint32Property(pclsObj, L"ConfiguredClockSpeed");
        if (configuredSpeed == 0) {
          configuredSpeed = speed;
        }

        std::string slotName = detail::GetSafeMemoryProperty(pclsObj, L"DeviceLocator", detail::kUnknownRamSlot);

        std::string manufacturer =
            detail::GetSafeMemoryProperty(pclsObj, L"Manufacturer", detail::kUnknownRamManufacturer);

        m_ramSlots.emplace_back(capacity, speed, configuredSpeed, std::move(slotName), std::move(manufacturer));
      } catch (...) {
      }

      if (pclsObj) {
        pclsObj->Release();
        pclsObj = nullptr;
      }
    }

    m_initialized = true;
    m_lastError = MemoryError::Success;
  } catch (...) {
    m_lastError = MemoryError::PropertyRetrievalFailed;
  }
}

uint64_t MemoryInfo::GetTotalPhysical() const noexcept { return m_totalPhys; }

uint64_t MemoryInfo::GetAvailablePhysical() const noexcept { return m_availPhys; }

uint64_t MemoryInfo::GetUsedPhysical() const noexcept { return m_usedPhys; }

uint32_t MemoryInfo::GetMemoryLoad() const noexcept { return m_memoryLoad; }

size_t MemoryInfo::GetRAMSlotCount() const noexcept { return m_ramSlots.size(); }

const RAMSlotInfo *MemoryInfo::GetRAMSlot(size_t index) const noexcept {
  if (index < m_ramSlots.size()) {
    return &m_ramSlots[index];
  }
  return nullptr;
}

const std::vector<RAMSlotInfo> &MemoryInfo::GetRAMSlots() const noexcept { return m_ramSlots; }

bool MemoryInfo::IsInitialized() const noexcept { return m_initialized; }

MemoryError MemoryInfo::GetLastError() const noexcept { return m_lastError; }

std::unique_ptr<MemoryInfo> GetMemoryInfo() { return std::make_unique<MemoryInfo>(); }

}  // namespace nysys