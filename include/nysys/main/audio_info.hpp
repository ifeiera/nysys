#ifndef AUDIO_INFO_HPP
#define AUDIO_INFO_HPP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nysys {
enum class AudioError {
  Success = 0,
  WMISessionFailed,
  QueryExecutionFailed,
  PropertyRetrievalFailed,
  InvalidParameter
};

[[nodiscard]] constexpr std::string_view ToString(AudioError error) noexcept {
  switch (error) {
    case AudioError::Success:
      return "Success";
    case AudioError::WMISessionFailed:
      return "WMI session initialization failed";
    case AudioError::QueryExecutionFailed:
      return "WMI query execution failed";
    case AudioError::PropertyRetrievalFailed:
      return "Audio property retrieval failed";
    case AudioError::InvalidParameter:
      return "Invalid parameter";
    default:
      return "Unknown error";
  }
}

template <typename T>
using AudioResult = std::optional<T>;

class AudioDeviceInfo {
public:
  AudioDeviceInfo() = default;
  AudioDeviceInfo(std::string deviceName, std::string deviceManufacturer) noexcept;

  [[nodiscard]] const std::string &GetName() const noexcept;
  [[nodiscard]] const std::string &GetManufacturer() const noexcept;

private:
  std::string m_name;
  std::string m_manufacturer;
};

class AudioList {
public:
  AudioList() noexcept;
  ~AudioList() = default;

  AudioList(AudioList &&other) noexcept = default;
  AudioList &operator=(AudioList &&other) noexcept = default;

  AudioList(const AudioList &) = delete;
  AudioList &operator=(const AudioList &) = delete;

  [[nodiscard]] size_t GetCount() const noexcept;
  [[nodiscard]] const AudioDeviceInfo *GetDevice(size_t index) const noexcept;
  [[nodiscard]] const std::vector<AudioDeviceInfo> &GetDevices() const noexcept;
  [[nodiscard]] bool IsInitialized() const noexcept;
  [[nodiscard]] AudioError GetLastError() const noexcept;

private:
  std::vector<AudioDeviceInfo> m_devices;
  bool m_initialized = false;
  AudioError m_lastError = AudioError::Success;

  void Initialize() noexcept;
};

[[nodiscard]] std::unique_ptr<AudioList> GetAudioDeviceList();

namespace detail {
constexpr std::string_view kUnknownAudioDevice = "Unknown Audio Device";
constexpr std::string_view kUnknownAudioManufacturer = "N/A";
}  // namespace detail

}  // namespace nysys

#endif
