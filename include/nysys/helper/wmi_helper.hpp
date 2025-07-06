#ifndef WMI_HELPER_HPP
#define WMI_HELPER_HPP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <memory>
#include <oleauto.h>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <wbemidl.h>
#include <wrl/client.h>

namespace wmi {
enum class WMIError {
  Success = 0,
  ComInitializationFailed,
  SecurityInitializationFailed,
  LocatorCreationFailed,
  ServerConnectionFailed,
  ProxySecurityFailed,
  QueryExecutionFailed,
  PropertyRetrievalFailed,
  InvalidParameter
};

[[nodiscard]] constexpr std::string_view ToString(WMIError error) noexcept {
  switch (error) {
    case WMIError::Success:
      return "Success";
    case WMIError::ComInitializationFailed:
      return "COM initialization failed";
    case WMIError::SecurityInitializationFailed:
      return "COM security initialization failed";
    case WMIError::LocatorCreationFailed:
      return "WMI locator creation failed";
    case WMIError::ServerConnectionFailed:
      return "WMI server connection failed";
    case WMIError::ProxySecurityFailed:
      return "WMI proxy security setup failed";
    case WMIError::QueryExecutionFailed:
      return "WMI query execution failed";
    case WMIError::PropertyRetrievalFailed:
      return "WMI property retrieval failed";
    case WMIError::InvalidParameter:
      return "Invalid parameter";
    default:
      return "Unknown error";
  }
}

template <typename T>
using WMIResult = std::optional<T>;

class WMISessionImpl;

class WMISession {
public:
  WMISession() noexcept;
  ~WMISession();

  WMISession(WMISession &&other) noexcept;
  WMISession &operator=(WMISession &&other) noexcept;

  WMISession(const WMISession &) = delete;
  WMISession &operator=(const WMISession &) = delete;

  [[nodiscard]] Microsoft::WRL::ComPtr<IEnumWbemClassObject> ExecuteQuery(std::wstring_view query) const noexcept;
  [[nodiscard]] bool IsInitialized() const noexcept;
  [[nodiscard]] WMIError GetLastError() const noexcept;

private:
  std::unique_ptr<WMISessionImpl> m_pImpl;
};

[[nodiscard]] std::string GetPropertyString(IWbemClassObject *pclsObj, std::wstring_view property) noexcept;

template <typename T>
[[nodiscard]] WMIResult<T> GetPropertyNumeric(IWbemClassObject *pclsObj, std::wstring_view property) noexcept;

namespace detail {
class VariantWrapper {
public:
  VariantWrapper() noexcept { VariantInit(&m_variant); }

  ~VariantWrapper() noexcept { VariantClear(&m_variant); }

  VariantWrapper(const VariantWrapper &) = delete;
  VariantWrapper &operator=(const VariantWrapper &) = delete;
  VariantWrapper(VariantWrapper &&) = delete;
  VariantWrapper &operator=(VariantWrapper &&) = delete;

  VARIANT *Get() noexcept { return &m_variant; }

  const VARIANT *Get() const noexcept { return &m_variant; }

  VARIANT *operator&() noexcept { return &m_variant; }

  const VARIANT &operator*() const noexcept { return m_variant; }

private:
  VARIANT m_variant;
};

[[nodiscard]] std::string BstrToUtf8(BSTR bstr) noexcept;
}  // namespace detail

}  // namespace wmi

#endif