#include "main/audio_info.hpp"

#include <algorithm>

#include "helper/wmi_helper.hpp"

namespace nysys {
namespace detail {

[[nodiscard]] std::string GetSafeStringProperty(IWbemClassObject *pclsObj, std::wstring_view property,
                                                std::string_view fallback) noexcept {
  if (!pclsObj) {
    return std::string{fallback};
  }

  std::string result = wmi::GetPropertyString(pclsObj, property);
  return result.empty() ? std::string{fallback} : result;
}
}  // namespace detail

AudioDeviceInfo::AudioDeviceInfo(std::string deviceName, std::string deviceManufacturer) noexcept
    : m_name(std::move(deviceName)), m_manufacturer(std::move(deviceManufacturer)) {}

const std::string &AudioDeviceInfo::GetName() const noexcept { return m_name; }

const std::string &AudioDeviceInfo::GetManufacturer() const noexcept { return m_manufacturer; }

AudioList::AudioList() noexcept { Initialize(); }

void AudioList::Initialize() noexcept {
  try {
    wmi::WMISession wmiSession;
    if (!wmiSession.IsInitialized()) {
      m_lastError = AudioError::WMISessionFailed;
      return;
    }

    auto pEnumerator = wmiSession.ExecuteQuery(L"SELECT Name, Manufacturer FROM Win32_SoundDevice");
    if (!pEnumerator) {
      m_lastError = AudioError::QueryExecutionFailed;
      return;
    }

    ULONG uReturn = 0;
    IWbemClassObject *pclsObj = nullptr;

    while (SUCCEEDED(pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn)) && uReturn != 0) {
      try {
        std::string deviceName = detail::GetSafeStringProperty(pclsObj, L"Name", detail::kUnknownAudioDevice);
        std::string manufacturer =
            detail::GetSafeStringProperty(pclsObj, L"Manufacturer", detail::kUnknownAudioManufacturer);
        m_devices.emplace_back(std::move(deviceName), std::move(manufacturer));
      } catch (...) {
      }

      if (pclsObj) {
        pclsObj->Release();
        pclsObj = nullptr;
      }
    }

    m_initialized = true;
    m_lastError = AudioError::Success;
  } catch (...) {
    m_lastError = AudioError::PropertyRetrievalFailed;
    m_devices.clear();
  }
}

size_t AudioList::GetCount() const noexcept { return m_devices.size(); }

const AudioDeviceInfo *AudioList::GetDevice(size_t index) const noexcept {
  if (index < m_devices.size()) {
    return &m_devices[index];
  }
  return nullptr;
}

const std::vector<AudioDeviceInfo> &AudioList::GetDevices() const noexcept { return m_devices; }

bool AudioList::IsInitialized() const noexcept { return m_initialized; }

AudioError AudioList::GetLastError() const noexcept { return m_lastError; }

std::unique_ptr<AudioList> GetAudioDeviceList() { return std::make_unique<AudioList>(); }

}  // namespace nysys