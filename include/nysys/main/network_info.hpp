#ifndef NETWORK_INFO_HPP
#define NETWORK_INFO_HPP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <cstdint>
#include <iphlpapi.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nysys {

enum class NetworkError { Success = 0, AdapterInfoFailed, MemoryAllocationFailed, BufferOverflow, InvalidParameter };

[[nodiscard]] constexpr std::string_view ToString(NetworkError error) noexcept {
  switch (error) {
    case NetworkError::Success:
      return "Success";
    case NetworkError::AdapterInfoFailed:
      return "Failed to retrieve adapter information";
    case NetworkError::MemoryAllocationFailed:
      return "Memory allocation failed";
    case NetworkError::BufferOverflow:
      return "Buffer overflow during adapter enumeration";
    case NetworkError::InvalidParameter:
      return "Invalid parameter";
    default:
      return "Unknown error";
  }
}

template <typename T>
using NetworkResult = std::optional<T>;

class NetworkAdapterInfo {
public:
  NetworkAdapterInfo() = default;

  NetworkAdapterInfo(std::string adapterName, std::string mac, std::string ip, std::string connStatus,
                     uint32_t adapterType) noexcept;

  [[nodiscard]] const std::string &GetName() const noexcept;
  [[nodiscard]] const std::string &GetMacAddress() const noexcept;
  [[nodiscard]] const std::string &GetIPAddress() const noexcept;
  [[nodiscard]] const std::string &GetStatus() const noexcept;
  [[nodiscard]] bool IsEthernet() const noexcept;
  [[nodiscard]] bool IsWiFi() const noexcept;

private:
  std::string m_name;
  std::string m_macAddress;
  std::string m_ipAddress;
  std::string m_status;
  uint32_t m_type = 0;
};

class NetworkList {
public:
  NetworkList() noexcept;

  ~NetworkList() = default;

  NetworkList(NetworkList &&other) noexcept = default;
  NetworkList &operator=(NetworkList &&other) noexcept = default;

  NetworkList(const NetworkList &) = delete;
  NetworkList &operator=(const NetworkList &) = delete;

  [[nodiscard]] size_t GetCount() const noexcept;
  [[nodiscard]] const NetworkAdapterInfo *GetAdapter(size_t index) const noexcept;
  [[nodiscard]] const std::vector<NetworkAdapterInfo> &GetAdapters() const noexcept;
  [[nodiscard]] bool IsInitialized() const noexcept;
  [[nodiscard]] NetworkError GetLastError() const noexcept;

private:
  std::vector<NetworkAdapterInfo> m_adapters;
  bool m_initialized = false;
  NetworkError m_lastError = NetworkError::Success;

  void Initialize() noexcept;

  [[nodiscard]] bool IsSystemAdapter(std::string_view description) const noexcept;
};

[[nodiscard]] std::unique_ptr<NetworkList> GetNetworkAdapterList();

namespace detail {

constexpr std::string_view kUnknownAdapter = "Unknown Adapter";
constexpr std::string_view kNoIpAddress = "N/A";
constexpr std::string_view kNotConnected = "Not Connected";
constexpr std::string_view kConnected = "Connected";
}  // namespace detail

}  // namespace nysys

#endif