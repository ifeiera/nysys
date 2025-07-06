#ifndef STORAGE_INFO_HPP
#define STORAGE_INFO_HPP

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
#include <string>
#include <string_view>
#include <vector>

namespace nysys {

enum class StorageError {
  Success = 0,
  WMISessionFailed,
  QueryExecutionFailed,
  PropertyRetrievalFailed,
  InvalidParameter
};

[[nodiscard]] constexpr std::string_view ToString(StorageError error) noexcept {
  switch (error) {
    case StorageError::Success:
      return "Success";
    case StorageError::WMISessionFailed:
      return "WMI session initialization failed";
    case StorageError::QueryExecutionFailed:
      return "WMI query execution failed";
    case StorageError::PropertyRetrievalFailed:
      return "Storage property retrieval failed";
    case StorageError::InvalidParameter:
      return "Invalid parameter";
    default:
      return "Unknown error";
  }
}

template <typename T>
using StorageResult = std::optional<T>;

class PhysicalDiskInfo {
public:
  PhysicalDiskInfo() = default;

  PhysicalDiskInfo(std::string diskModel, std::string diskInterface, std::string diskDeviceID) noexcept;

  [[nodiscard]] const std::string &GetModel() const noexcept;
  [[nodiscard]] const std::string &GetInterfaceType() const noexcept;
  [[nodiscard]] const std::string &GetDeviceID() const noexcept;

private:
  std::string m_model;
  std::string m_interfaceType;
  std::string m_deviceID;
};

class LogicalDiskInfo {
public:
  LogicalDiskInfo() = default;

  LogicalDiskInfo(std::string driveLetter, std::string driveType, std::string driveModel, std::string diskInterface,
                  double diskTotalSize, double diskFreeSpace) noexcept;

  [[nodiscard]] const std::string &GetDriveLetter() const noexcept;
  [[nodiscard]] const std::string &GetType() const noexcept;
  [[nodiscard]] const std::string &GetModel() const noexcept;
  [[nodiscard]] const std::string &GetInterfaceType() const noexcept;
  [[nodiscard]] double GetTotalSize() const noexcept;
  [[nodiscard]] double GetAvailableSpace() const noexcept;

private:
  std::string m_drive;
  std::string m_type;
  std::string m_model;
  std::string m_interfaceType;
  double m_totalSize = 0.0;
  double m_freeSpace = 0.0;
};

class StorageList {
public:
  StorageList() noexcept;

  ~StorageList() = default;

  StorageList(StorageList &&other) noexcept = default;
  StorageList &operator=(StorageList &&other) noexcept = default;

  StorageList(const StorageList &) = delete;
  StorageList &operator=(const StorageList &) = delete;

  [[nodiscard]] size_t GetCount() const noexcept;
  [[nodiscard]] const LogicalDiskInfo *GetDisk(size_t index) const noexcept;
  [[nodiscard]] const std::vector<LogicalDiskInfo> &GetDisks() const noexcept;
  [[nodiscard]] bool IsInitialized() const noexcept;
  [[nodiscard]] StorageError GetLastError() const noexcept;

private:
  std::vector<LogicalDiskInfo> m_disks;
  bool m_initialized = false;
  StorageError m_lastError = StorageError::Success;

  void Initialize() noexcept;
};

[[nodiscard]] std::unique_ptr<StorageList> GetStorageList();

namespace detail {

constexpr std::string_view kUnknownStorageDevice = "Unknown Storage Device";
constexpr std::string_view kUnknownInterface = "Unknown";
constexpr std::string_view kUnknownDriveType = "Unknown";
}  // namespace detail

}  // namespace nysys

#endif