#include "main/storage_info.hpp"

#include <algorithm>
#include <memory>

#include "helper/utils.hpp"
#include "helper/wmi_helper.hpp"

namespace nysys {
namespace {

[[nodiscard]] std::string GetSafeStringProperty(IWbemClassObject *pclsObj, std::wstring_view property,
                                                std::string_view fallback) noexcept {
  if (!pclsObj) {
    return std::string{fallback};
  }

  std::string result = wmi::GetPropertyString(pclsObj, property);
  return result.empty() ? std::string{fallback} : result;
}

[[nodiscard]] double GetSafeDoubleProperty(IWbemClassObject *pclsObj, std::wstring_view property,
                                           double fallback = 0.0) noexcept {
  if (!pclsObj) {
    return fallback;
  }

  VARIANT vtProp;
  VariantInit(&vtProp);

  if (SUCCEEDED(pclsObj->Get(property.data(), 0, &vtProp, 0, 0)) && vtProp.vt != VT_NULL) {
    double result = fallback;
    if (vtProp.vt == VT_BSTR) {
      result = utils::DoubleToGB(_wtof(vtProp.bstrVal));
    }
    VariantClear(&vtProp);
    return result;
  }

  VariantClear(&vtProp);
  return fallback;
}

[[nodiscard]] std::string GetDriveTypeString(UINT driveType) noexcept {
  switch (driveType) {
    case 2:
      return "Removable Disk";
    case 3:
      return "Local Disk";
    case 4:
      return "Network Drive";
    case 5:
      return "CD/DVD Drive";
    case 6:
      return "RAM Disk";
    default:
      return std::string{detail::kUnknownDriveType};
  }
}
}  // namespace

PhysicalDiskInfo::PhysicalDiskInfo(std::string diskModel, std::string diskInterface, std::string diskDeviceID) noexcept
    : m_model(std::move(diskModel)), m_interfaceType(std::move(diskInterface)), m_deviceID(std::move(diskDeviceID)) {}

const std::string &PhysicalDiskInfo::GetModel() const noexcept { return m_model; }

const std::string &PhysicalDiskInfo::GetInterfaceType() const noexcept { return m_interfaceType; }

const std::string &PhysicalDiskInfo::GetDeviceID() const noexcept { return m_deviceID; }

LogicalDiskInfo::LogicalDiskInfo(std::string driveLetter, std::string driveType, std::string driveModel,
                                 std::string diskInterface, double diskTotalSize, double diskFreeSpace) noexcept
    : m_drive(std::move(driveLetter)),
      m_type(std::move(driveType)),
      m_model(std::move(driveModel)),
      m_interfaceType(std::move(diskInterface)),
      m_totalSize(diskTotalSize),
      m_freeSpace(diskFreeSpace) {}

const std::string &LogicalDiskInfo::GetDriveLetter() const noexcept { return m_drive; }

const std::string &LogicalDiskInfo::GetType() const noexcept { return m_type; }

const std::string &LogicalDiskInfo::GetModel() const noexcept { return m_model; }

const std::string &LogicalDiskInfo::GetInterfaceType() const noexcept { return m_interfaceType; }

double LogicalDiskInfo::GetTotalSize() const noexcept { return m_totalSize; }

double LogicalDiskInfo::GetAvailableSpace() const noexcept { return m_freeSpace; }

static void ProcessLogicalDisk(IWbemClassObject *pLogicalObj, const std::string &physicalModel,
                               const std::string &physicalInterface, std::vector<LogicalDiskInfo> &disks) noexcept {
  try {
    std::string driveLetter = GetSafeStringProperty(pLogicalObj, L"DeviceID", "N/A");

    if (driveLetter == "N/A") {
      return;
    }

    VARIANT vtDriveType;
    VariantInit(&vtDriveType);
    std::string driveType = std::string{detail::kUnknownDriveType};

    if (SUCCEEDED(pLogicalObj->Get(L"DriveType", 0, &vtDriveType, 0, 0)) && vtDriveType.vt != VT_NULL) {
      driveType = GetDriveTypeString(vtDriveType.uintVal);
    }
    VariantClear(&vtDriveType);

    std::string volumeName = GetSafeStringProperty(pLogicalObj, L"VolumeName", "");

    std::string model = volumeName.empty() ? physicalModel : volumeName;
    if (model.empty()) {
      model = std::string{detail::kUnknownStorageDevice};
    }

    double totalSize = GetSafeDoubleProperty(pLogicalObj, L"Size");
    double freeSpace = GetSafeDoubleProperty(pLogicalObj, L"FreeSpace");

    disks.emplace_back(std::move(driveLetter), std::move(driveType), std::move(model), std::string{physicalInterface},
                       totalSize, freeSpace);
  } catch (...) {
  }
}

StorageList::StorageList() noexcept { Initialize(); }

void StorageList::Initialize() noexcept {
  try {
    wmi::WMISession wmiSession;
    if (!wmiSession.IsInitialized()) {
      m_lastError = StorageError::WMISessionFailed;
      return;
    }

    auto pEnumerator = wmiSession.ExecuteQuery(L"SELECT * FROM Win32_DiskDrive");
    if (!pEnumerator) {
      m_lastError = StorageError::QueryExecutionFailed;
      return;
    }

    ULONG uReturn = 0;
    IWbemClassObject *pDiskObj = nullptr;

    while (SUCCEEDED(pEnumerator->Next(WBEM_INFINITE, 1, &pDiskObj, &uReturn)) && uReturn != 0) {
      try {
        std::string model = GetSafeStringProperty(pDiskObj, L"Model", detail::kUnknownStorageDevice);

        std::string interfaceType = GetSafeStringProperty(pDiskObj, L"InterfaceType", detail::kUnknownInterface);

        VARIANT vtDeviceID;
        VariantInit(&vtDeviceID);

        if (SUCCEEDED(pDiskObj->Get(L"DeviceID", 0, &vtDeviceID, 0, 0)) && vtDeviceID.vt == VT_BSTR) {
          std::wstring partQuery = L"ASSOCIATORS OF {Win32_DiskDrive.DeviceID='";
          partQuery += vtDeviceID.bstrVal;
          partQuery += L"'} WHERE AssocClass = Win32_DiskDriveToDiskPartition";

          auto partEnum = wmiSession.ExecuteQuery(partQuery);
          if (partEnum) {
            IWbemClassObject *pPartObj = nullptr;
            ULONG uPartReturn = 0;

            while (SUCCEEDED(partEnum->Next(WBEM_INFINITE, 1, &pPartObj, &uPartReturn)) && uPartReturn != 0) {
              VARIANT vtPartID;
              VariantInit(&vtPartID);

              if (SUCCEEDED(pPartObj->Get(L"DeviceID", 0, &vtPartID, 0, 0)) && vtPartID.vt == VT_BSTR) {
                std::wstring logicalQuery = L"ASSOCIATORS OF {Win32_DiskPartition.DeviceID='";
                logicalQuery += vtPartID.bstrVal;
                logicalQuery += L"'} WHERE AssocClass = Win32_LogicalDiskToPartition";

                auto logicalEnum = wmiSession.ExecuteQuery(logicalQuery);
                if (logicalEnum) {
                  IWbemClassObject *pLogicalObj = nullptr;
                  ULONG uLogicalReturn = 0;

                  while (SUCCEEDED(logicalEnum->Next(WBEM_INFINITE, 1, &pLogicalObj, &uLogicalReturn)) &&
                         uLogicalReturn != 0) {
                    ProcessLogicalDisk(pLogicalObj, model, interfaceType, m_disks);

                    if (pLogicalObj) {
                      pLogicalObj->Release();
                      pLogicalObj = nullptr;
                    }
                  }
                }
              }

              VariantClear(&vtPartID);
              if (pPartObj) {
                pPartObj->Release();
                pPartObj = nullptr;
              }
            }
          }
        }

        VariantClear(&vtDeviceID);
      } catch (...) {
      }

      if (pDiskObj) {
        pDiskObj->Release();
        pDiskObj = nullptr;
      }
    }

    m_initialized = true;
    m_lastError = StorageError::Success;
  } catch (...) {
    m_lastError = StorageError::PropertyRetrievalFailed;
    m_disks.clear();
  }
}

size_t StorageList::GetCount() const noexcept { return m_disks.size(); }

const LogicalDiskInfo *StorageList::GetDisk(size_t index) const noexcept {
  if (index < m_disks.size()) {
    return &m_disks[index];
  }
  return nullptr;
}

const std::vector<LogicalDiskInfo> &StorageList::GetDisks() const noexcept { return m_disks; }

bool StorageList::IsInitialized() const noexcept { return m_initialized; }

StorageError StorageList::GetLastError() const noexcept { return m_lastError; }

std::unique_ptr<StorageList> GetStorageList() { return std::make_unique<StorageList>(); }

}  // namespace nysys