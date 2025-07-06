#include "helper/wmi_helper.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <type_traits>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "ole32.lib")

namespace wmi {
namespace detail {
std::string BstrToUtf8(BSTR bstr) noexcept {
  if (!bstr) {
    return {};
  }

  const int size_needed = WideCharToMultiByte(CP_UTF8, 0, bstr, -1, nullptr, 0, nullptr, nullptr);

  if (size_needed <= 1) {
    return {};
  }

  std::string result(size_needed - 1, '\0');
  WideCharToMultiByte(CP_UTF8, 0, bstr, -1, result.data(), size_needed, nullptr, nullptr);

  return result;
}

WMIError InitializeCOM(bool &comInitialized) noexcept {
  comInitialized = false;

  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  if (hr == S_OK) {
    comInitialized = true;
    return WMIError::Success;
  }

  if (hr == S_FALSE) {
    return WMIError::Success;
  }

  if (hr == RPC_E_CHANGED_MODE) {
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (hr == S_OK) {
      comInitialized = true;
      return WMIError::Success;
    }

    if (hr == S_FALSE) {
      return WMIError::Success;
    }
  }

  return WMIError::ComInitializationFailed;
}

WMIError InitializeCOMSecurity() noexcept {
  const HRESULT hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT,
                                          RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);

  if (SUCCEEDED(hr) || hr == RPC_E_TOO_LATE) {
    return WMIError::Success;
  }

  return WMIError::SecurityInitializationFailed;
}
}  // namespace detail

class WMISessionImpl {
public:
  WMISessionImpl() noexcept : m_initialized(false), m_comInitialized(false), m_lastError(WMIError::Success) {}

  ~WMISessionImpl() noexcept { Cleanup(); }

  WMISessionImpl(WMISessionImpl &&other) noexcept
      : m_wmiService(std::move(other.m_wmiService)),
        m_wmiLocator(std::move(other.m_wmiLocator)),
        m_initialized(other.m_initialized),
        m_comInitialized(other.m_comInitialized),
        m_lastError(other.m_lastError) {
    other.m_initialized = false;
    other.m_comInitialized = false;
    other.m_lastError = WMIError::Success;
  }

  WMISessionImpl &operator=(WMISessionImpl &&other) noexcept {
    if (this != &other) {
      Cleanup();

      m_wmiService = std::move(other.m_wmiService);
      m_wmiLocator = std::move(other.m_wmiLocator);
      m_initialized = other.m_initialized;
      m_comInitialized = other.m_comInitialized;
      m_lastError = other.m_lastError;

      other.m_initialized = false;
      other.m_comInitialized = false;
      other.m_lastError = WMIError::Success;
    }
    return *this;
  }

  WMISessionImpl(const WMISessionImpl &) = delete;
  WMISessionImpl &operator=(const WMISessionImpl &) = delete;

  WMIError Initialize() noexcept {
    m_lastError = detail::InitializeCOM(m_comInitialized);
    if (m_lastError != WMIError::Success) {
      return m_lastError;
    }

    m_lastError = detail::InitializeCOMSecurity();
    if (m_lastError != WMIError::Success) {
      Cleanup();
      return m_lastError;
    }

    HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                                  reinterpret_cast<void **>(m_wmiLocator.GetAddressOf()));

    if (FAILED(hr)) {
      m_lastError = WMIError::LocatorCreationFailed;
      Cleanup();
      return m_lastError;
    }

    hr = m_wmiLocator->ConnectServer(const_cast<BSTR>(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr,
                                     m_wmiService.GetAddressOf());

    if (FAILED(hr)) {
      m_lastError = WMIError::ServerConnectionFailed;
      Cleanup();
      return m_lastError;
    }

    hr = CoSetProxyBlanket(m_wmiService.Get(), RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

    if (FAILED(hr)) {
      m_lastError = WMIError::ProxySecurityFailed;
      Cleanup();
      return m_lastError;
    }

    m_initialized = true;
    m_lastError = WMIError::Success;
    return WMIError::Success;
  }

  void Cleanup() noexcept {
    m_wmiService.Reset();
    m_wmiLocator.Reset();

    if (m_comInitialized) {
      CoUninitialize();
      m_comInitialized = false;
    }

    m_initialized = false;
  }

  Microsoft::WRL::ComPtr<IWbemServices> m_wmiService;
  Microsoft::WRL::ComPtr<IWbemLocator> m_wmiLocator;
  bool m_initialized;
  bool m_comInitialized;
  WMIError m_lastError;
};

WMISession::WMISession() noexcept : m_pImpl(std::make_unique<WMISessionImpl>()) {
  if (m_pImpl) {
    m_pImpl->Initialize();
  }
}

WMISession::~WMISession() = default;

WMISession::WMISession(WMISession &&other) noexcept = default;

WMISession &WMISession::operator=(WMISession &&other) noexcept = default;

bool WMISession::IsInitialized() const noexcept { return m_pImpl && m_pImpl->m_initialized; }

WMIError WMISession::GetLastError() const noexcept {
  return m_pImpl ? m_pImpl->m_lastError : WMIError::InvalidParameter;
}

Microsoft::WRL::ComPtr<IEnumWbemClassObject> WMISession::ExecuteQuery(std::wstring_view query) const noexcept {
  if (!IsInitialized() || query.empty()) {
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IEnumWbemClassObject> enumerator;

  const std::wstring queryStr(query);

  const HRESULT hr = m_pImpl->m_wmiService->ExecQuery(const_cast<BSTR>(L"WQL"), const_cast<BSTR>(queryStr.c_str()),
                                                      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
                                                      enumerator.GetAddressOf());

  if (FAILED(hr)) {
    return nullptr;
  }

  return enumerator;
}

std::string GetPropertyString(IWbemClassObject *pclsObj, std::wstring_view property) noexcept {
  if (!pclsObj || property.empty()) {
    return {};
  }

  detail::VariantWrapper variant;

  const std::wstring propertyStr(property);

  const HRESULT hr = pclsObj->Get(propertyStr.c_str(), 0, variant.Get(), nullptr, nullptr);
  if (FAILED(hr) || variant.Get()->vt == VT_NULL || variant.Get()->vt == VT_EMPTY) {
    return {};
  }

  const VARIANT *pVariant = variant.Get();
  switch (pVariant->vt) {
    case VT_BSTR:
      return detail::BstrToUtf8(pVariant->bstrVal);
    case VT_I4:
      return std::to_string(pVariant->lVal);
    case VT_UI4:
      return std::to_string(pVariant->ulVal);
    case VT_I8:
      return std::to_string(pVariant->llVal);
    case VT_UI8:
      return std::to_string(pVariant->ullVal);
    case VT_R4:
      return std::to_string(pVariant->fltVal);
    case VT_R8:
      return std::to_string(pVariant->dblVal);
    case VT_BOOL:
      return (pVariant->boolVal != VARIANT_FALSE) ? "true" : "false";
    default:
      detail::VariantWrapper stringVariant;
      if (SUCCEEDED(VariantChangeType(stringVariant.Get(), variant.Get(), 0, VT_BSTR))) {
        return detail::BstrToUtf8(stringVariant.Get()->bstrVal);
      }
      return {};
  }
}

namespace detail {
template <typename T>
WMIResult<T> ExtractNumericValue(const VARIANT &vtProp) noexcept {
  static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

  switch (vtProp.vt) {
    case VT_I1:
      return static_cast<T>(vtProp.cVal);
    case VT_UI1:
      return static_cast<T>(vtProp.bVal);
    case VT_I2:
      return static_cast<T>(vtProp.iVal);
    case VT_UI2:
      return static_cast<T>(vtProp.uiVal);
    case VT_I4:
      return static_cast<T>(vtProp.lVal);
    case VT_UI4:
      return static_cast<T>(vtProp.ulVal);
    case VT_I8:
      return static_cast<T>(vtProp.llVal);
    case VT_UI8:
      return static_cast<T>(vtProp.ullVal);
    case VT_R4:
      return static_cast<T>(vtProp.fltVal);
    case VT_R8:
      return static_cast<T>(vtProp.dblVal);
    case VT_BOOL:
      if constexpr (std::is_same_v<T, bool>) {
        return vtProp.boolVal != VARIANT_FALSE;
      } else {
        return static_cast<T>(vtProp.boolVal != VARIANT_FALSE ? 1 : 0);
      }
    default:
      return std::nullopt;
  }
}
}  // namespace detail

template <typename T>
WMIResult<T> GetPropertyNumeric(IWbemClassObject *pclsObj, std::wstring_view property) noexcept {
  static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

  if (!pclsObj || property.empty()) {
    return std::nullopt;
  }

  detail::VariantWrapper variant;

  const std::wstring propertyStr(property);

  const HRESULT hr = pclsObj->Get(propertyStr.c_str(), 0, variant.Get(), nullptr, nullptr);
  if (FAILED(hr) || variant.Get()->vt == VT_NULL || variant.Get()->vt == VT_EMPTY) {
    return std::nullopt;
  }

  return detail::ExtractNumericValue<T>(*variant.Get());
}

template WMIResult<int> GetPropertyNumeric<int>(IWbemClassObject *, std::wstring_view) noexcept;
template WMIResult<long> GetPropertyNumeric<long>(IWbemClassObject *, std::wstring_view) noexcept;
template WMIResult<long long> GetPropertyNumeric<long long>(IWbemClassObject *, std::wstring_view) noexcept;
template WMIResult<unsigned int> GetPropertyNumeric<unsigned int>(IWbemClassObject *, std::wstring_view) noexcept;
template WMIResult<unsigned long> GetPropertyNumeric<unsigned long>(IWbemClassObject *, std::wstring_view) noexcept;
template WMIResult<unsigned long long> GetPropertyNumeric<unsigned long long>(IWbemClassObject *,
                                                                              std::wstring_view) noexcept;
template WMIResult<float> GetPropertyNumeric<float>(IWbemClassObject *, std::wstring_view) noexcept;
template WMIResult<double> GetPropertyNumeric<double>(IWbemClassObject *, std::wstring_view) noexcept;
template WMIResult<bool> GetPropertyNumeric<bool>(IWbemClassObject *, std::wstring_view) noexcept;

}  // namespace wmi