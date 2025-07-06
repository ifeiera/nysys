#include "main/motherboard_info.hpp"

#include <algorithm>

#include "helper/wmi_helper.hpp"

namespace nysys {
namespace detail {

[[nodiscard]] std::string GetSafeMotherboardProperty(IWbemClassObject *pclsObj, std::wstring_view property,
                                                     std::string_view fallback) noexcept {
  if (!pclsObj) {
    return std::string{fallback};
  }

  std::string result = wmi::GetPropertyString(pclsObj, property);
  return result.empty() ? std::string{fallback} : result;
}

[[nodiscard]] IWbemClassObject *GetFirstWMIObject(wmi::WMISession &session, std::wstring_view query) noexcept {
  auto pEnumerator = session.ExecuteQuery(query);
  if (!pEnumerator) {
    return nullptr;
  }

  IWbemClassObject *pclsObj = nullptr;
  ULONG uReturn = 0;

  if (SUCCEEDED(pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn)) && uReturn != 0) {
    return pclsObj;
  }

  return nullptr;
}
}  // namespace detail

MotherboardInfo::MotherboardInfo() noexcept { Initialize(); }

void MotherboardInfo::Initialize() noexcept {
  try {
    m_productName = std::string{detail::kUnknownMotherboardProduct};
    m_manufacturer = std::string{detail::kUnknownMotherboardManufacturer};
    m_serialNumber = std::string{detail::kUnknownMotherboardSerial};
    m_biosVersion = std::string{detail::kUnknownMotherboardBiosVersion};
    m_biosSerial = std::string{detail::kUnknownMotherboardBiosSerial};
    m_systemSKU = std::string{detail::kUnknownMotherboardSystemSKU};

    wmi::WMISession wmiSession;
    if (!wmiSession.IsInitialized()) {
      m_lastError = MotherboardError::WMISessionFailed;
      return;
    }

    if (auto pclsObj = detail::GetFirstWMIObject(wmiSession,
                                                 L"SELECT Product, Manufacturer, SerialNumber FROM "
                                                 L"Win32_BaseBoard")) {
      try {
        m_productName = detail::GetSafeMotherboardProperty(pclsObj, L"Product", detail::kUnknownMotherboardProduct);

        m_manufacturer =
            detail::GetSafeMotherboardProperty(pclsObj, L"Manufacturer", detail::kUnknownMotherboardManufacturer);

        m_serialNumber =
            detail::GetSafeMotherboardProperty(pclsObj, L"SerialNumber", detail::kUnknownMotherboardSerial);
      } catch (...) {
      }

      pclsObj->Release();
    }

    if (auto pclsObj =
            detail::GetFirstWMIObject(wmiSession, L"SELECT SMBIOSBIOSVersion, SerialNumber FROM Win32_BIOS")) {
      try {
        m_biosVersion =
            detail::GetSafeMotherboardProperty(pclsObj, L"SMBIOSBIOSVersion", detail::kUnknownMotherboardBiosVersion);

        m_biosSerial =
            detail::GetSafeMotherboardProperty(pclsObj, L"SerialNumber", detail::kUnknownMotherboardBiosSerial);
      } catch (...) {
      }

      pclsObj->Release();
    }

    if (auto pclsObj = detail::GetFirstWMIObject(wmiSession, L"SELECT SystemSKUNumber FROM Win32_ComputerSystem")) {
      try {
        m_systemSKU =
            detail::GetSafeMotherboardProperty(pclsObj, L"SystemSKUNumber", detail::kUnknownMotherboardSystemSKU);
      } catch (...) {
      }

      pclsObj->Release();
    }

    m_initialized = true;
    m_lastError = MotherboardError::Success;
  } catch (...) {
    m_lastError = MotherboardError::PropertyRetrievalFailed;
  }
}

const std::string &MotherboardInfo::GetProduct() const noexcept { return m_productName; }

const std::string &MotherboardInfo::GetManufacturer() const noexcept { return m_manufacturer; }

const std::string &MotherboardInfo::GetSerial() const noexcept { return m_serialNumber; }

const std::string &MotherboardInfo::GetBiosVersion() const noexcept { return m_biosVersion; }

const std::string &MotherboardInfo::GetBiosSerial() const noexcept { return m_biosSerial; }

const std::string &MotherboardInfo::GetSystemSKU() const noexcept { return m_systemSKU; }

bool MotherboardInfo::IsInitialized() const noexcept { return m_initialized; }

MotherboardError MotherboardInfo::GetLastError() const noexcept { return m_lastError; }

std::unique_ptr<MotherboardInfo> GetMotherboardInfo() { return std::make_unique<MotherboardInfo>(); }

}  // namespace nysys