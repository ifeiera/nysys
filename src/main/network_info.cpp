#include "main/network_info.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "iphlpapi.lib")

namespace nysys {
namespace detail {

[[nodiscard]] std::string FormatMacAddress(const BYTE *address, UINT length) noexcept {
  if (!address || length == 0) {
    return {};
  }

  std::ostringstream oss;
  oss << std::hex << std::uppercase << std::setfill('0');

  for (UINT i = 0; i < length; ++i) {
    if (i > 0) {
      oss << ':';
    }
    oss << std::setw(2) << static_cast<unsigned>(address[i]);
  }

  return oss.str();
}

[[nodiscard]] bool IsValidIpAddress(const char *ipStr) noexcept { return ipStr && strcmp(ipStr, "0.0.0.0") != 0; }
}  // namespace detail

NetworkAdapterInfo::NetworkAdapterInfo(std::string adapterName, std::string mac, std::string ip, std::string connStatus,
                                       uint32_t adapterType) noexcept
    : m_name(std::move(adapterName)),
      m_macAddress(std::move(mac)),
      m_ipAddress(std::move(ip)),
      m_status(std::move(connStatus)),
      m_type(adapterType) {}

const std::string &NetworkAdapterInfo::GetName() const noexcept { return m_name; }

const std::string &NetworkAdapterInfo::GetMacAddress() const noexcept { return m_macAddress; }

const std::string &NetworkAdapterInfo::GetIPAddress() const noexcept { return m_ipAddress; }

const std::string &NetworkAdapterInfo::GetStatus() const noexcept { return m_status; }

bool NetworkAdapterInfo::IsEthernet() const noexcept { return m_type == MIB_IF_TYPE_ETHERNET; }

bool NetworkAdapterInfo::IsWiFi() const noexcept { return m_type == IF_TYPE_IEEE80211; }

NetworkList::NetworkList() noexcept { Initialize(); }

void NetworkList::Initialize() noexcept {
  try {
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

    auto pAdapterInfo = std::make_unique<BYTE[]>(ulOutBufLen);
    auto pAdapterInfoStruct = reinterpret_cast<PIP_ADAPTER_INFO>(pAdapterInfo.get());

    DWORD result = GetAdaptersInfo(pAdapterInfoStruct, &ulOutBufLen);

    if (result == ERROR_BUFFER_OVERFLOW) {
      pAdapterInfo = std::make_unique<BYTE[]>(ulOutBufLen);
      pAdapterInfoStruct = reinterpret_cast<PIP_ADAPTER_INFO>(pAdapterInfo.get());
      result = GetAdaptersInfo(pAdapterInfoStruct, &ulOutBufLen);
    }

    if (result != NO_ERROR) {
      m_lastError = NetworkError::AdapterInfoFailed;
      return;
    }

    PIP_ADAPTER_INFO pAdapter = pAdapterInfoStruct;
    while (pAdapter) {
      try {
        if (!IsSystemAdapter(pAdapter->Description)) {
          std::string macAddress = detail::FormatMacAddress(pAdapter->Address, pAdapter->AddressLength);

          std::string ipAddress{detail::kNoIpAddress};
          std::string status{detail::kNotConnected};

          if (detail::IsValidIpAddress(pAdapter->IpAddressList.IpAddress.String)) {
            ipAddress = pAdapter->IpAddressList.IpAddress.String;
            status = detail::kConnected;
          }

          m_adapters.emplace_back(pAdapter->Description ? pAdapter->Description : std::string{detail::kUnknownAdapter},
                                  std::move(macAddress), std::move(ipAddress), std::move(status), pAdapter->Type);
        }
      } catch (...) {
      }

      pAdapter = pAdapter->Next;
    }

    m_initialized = true;
    m_lastError = NetworkError::Success;
  } catch (...) {
    m_lastError = NetworkError::MemoryAllocationFailed;
    m_adapters.clear();
  }
}

bool NetworkList::IsSystemAdapter(std::string_view description) const noexcept {
  if (description.empty()) {
    return false;
  }

  std::string lowerDesc;
  lowerDesc.reserve(description.size());
  std::transform(description.begin(), description.end(), std::back_inserter(lowerDesc),
                 [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

  const std::string_view lowerDescView{lowerDesc};
  return (lowerDescView.find("virtual") != std::string_view::npos ||
          lowerDescView.find("pseudo") != std::string_view::npos ||
          lowerDescView.find("loopback") != std::string_view::npos ||
          lowerDescView.find("microsoft") != std::string_view::npos);
}

size_t NetworkList::GetCount() const noexcept { return m_adapters.size(); }

const NetworkAdapterInfo *NetworkList::GetAdapter(size_t index) const noexcept {
  if (index < m_adapters.size()) {
    return &m_adapters[index];
  }
  return nullptr;
}

const std::vector<NetworkAdapterInfo> &NetworkList::GetAdapters() const noexcept { return m_adapters; }

bool NetworkList::IsInitialized() const noexcept { return m_initialized; }

NetworkError NetworkList::GetLastError() const noexcept { return m_lastError; }

std::unique_ptr<NetworkList> GetNetworkAdapterList() { return std::make_unique<NetworkList>(); }

}  // namespace nysys