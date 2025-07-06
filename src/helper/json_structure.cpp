#include "helper/json_structure.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "helper/utils.hpp"
#include "main/audio_info.hpp"
#include "main/battery_info.hpp"
#include "main/cpu_info.hpp"
#include "main/gpu_info.hpp"
#include "main/memory_info.hpp"
#include "main/monitor_info.hpp"
#include "main/motherboard_info.hpp"
#include "main/network_info.hpp"
#include "main/storage_info.hpp"

namespace json {

static std::string TrimString(const std::string &str) noexcept {
  if (str.empty()) {
    return str;
  }

  const auto first = str.find_first_not_of(" \t\n\r\f\v");
  if (first == std::string::npos) {
    return {};
  }

  const auto last = str.find_last_not_of(" \t\n\r\f\v");
  return str.substr(first, last - first + 1);
}

static void AppendGpuInfo(nlohmann::json &root, const nysys::GPUList *gpuList) noexcept {
  if (!gpuList) {
    return;
  }

  try {
    const auto &gpus = gpuList->GetGPUs();
    nlohmann::json gpuArray = nlohmann::json::array();

    for (const auto &gpu : gpus) {
      nlohmann::json gpuObj;
      gpuObj["name"] = gpu.GetName();
      gpuObj["vram"] = gpu.GetDedicatedMemory();
      gpuObj["shared_memory"] = gpu.GetSharedMemory();
      gpuObj["type"] = gpu.IsIntegrated() ? "iGPU" : "dGPU";
      gpuArray.push_back(gpuObj);
    }

    root["gpu"] = gpuArray;
  } catch (...) {
  }
}

static void AppendMotherboardInfo(nlohmann::json &root, const nysys::MotherboardInfo *mbInfo) noexcept {
  if (!mbInfo) {
    return;
  }

  try {
    nlohmann::json mbObj;
    mbObj["manufacturer"] = mbInfo->GetManufacturer();
    mbObj["product"] = mbInfo->GetProduct();
    mbObj["serial_number"] = mbInfo->GetSerial();
    mbObj["bios_version"] = mbInfo->GetBiosVersion();
    mbObj["bios_serial"] = mbInfo->GetBiosSerial();
    mbObj["system_sku"] = mbInfo->GetSystemSKU();

    root["motherboard"] = mbObj;
  } catch (...) {
  }
}

static void AppendCpuInfo(nlohmann::json &root, const nysys::CPUList *cpuList) noexcept {
  if (!cpuList) {
    return;
  }

  try {
    const auto &cpus = cpuList->GetCPUs();
    nlohmann::json cpuArray = nlohmann::json::array();

    for (const auto &cpu : cpus) {
      nlohmann::json cpuObj;
      cpuObj["name"] = TrimString(cpu.GetName());
      cpuObj["cores"] = cpu.GetCores();
      cpuObj["threads"] = cpu.GetThreads();
      cpuObj["clock_speed"] = cpu.GetClockSpeed();
      cpuArray.push_back(cpuObj);
    }

    root["cpu"] = cpuArray;
  } catch (...) {
  }
}

static void AppendMemoryInfo(nlohmann::json &root, const nysys::MemoryInfo *memInfo) noexcept {
  if (!memInfo) {
    return;
  }

  try {
    nlohmann::json memObj;
    memObj["total"] = utils::BytesToGB(memInfo->GetTotalPhysical());
    memObj["available"] = utils::BytesToGB(memInfo->GetAvailablePhysical());
    memObj["used"] = utils::BytesToGB(memInfo->GetUsedPhysical());
    memObj["usage_percent"] = memInfo->GetMemoryLoad();

    const auto &slots = memInfo->GetRAMSlots();
    nlohmann::json slotsArray = nlohmann::json::array();

    for (const auto &slot : slots) {
      nlohmann::json slotObj;
      slotObj["location"] = slot.GetSlotLocation();
      slotObj["capacity"] = utils::BytesToGB(slot.GetCapacity());
      slotObj["speed"] = slot.GetSpeed();
      slotObj["configured_speed"] = slot.GetConfiguredSpeed();
      slotObj["manufacturer"] = slot.GetManufacturer();
      slotsArray.push_back(slotObj);
    }

    memObj["ram_slots"] = slotsArray;
    root["memory"] = memObj;
  } catch (...) {
  }
}

static void AppendStorageInfo(nlohmann::json &root, const nysys::StorageList *storageList) noexcept {
  if (!storageList) {
    return;
  }

  try {
    const auto &disks = storageList->GetDisks();
    nlohmann::json storageArray = nlohmann::json::array();

    for (const auto &disk : disks) {
      nlohmann::json diskObj;
      const double totalSize = disk.GetTotalSize();
      const double freeSpace = disk.GetAvailableSpace();
      const double usedSpace = utils::RoundToDecimalPlaces(totalSize - freeSpace, 2);

      diskObj["drive"] = disk.GetDriveLetter();
      diskObj["type"] = disk.GetType();
      diskObj["model"] = disk.GetModel();
      diskObj["interface"] = disk.GetInterfaceType();
      diskObj["total_size"] = totalSize;
      diskObj["free_space"] = freeSpace;
      diskObj["used_space"] = usedSpace;
      storageArray.push_back(diskObj);
    }

    root["storage"] = storageArray;
  } catch (...) {
  }
}

static void AppendNetworkInfo(nlohmann::json &root, const nysys::NetworkList *networkList) noexcept {
  if (!networkList) {
    return;
  }

  try {
    const auto &adapters = networkList->GetAdapters();
    nlohmann::json networkObj;
    nlohmann::json ethernetArray = nlohmann::json::array();
    nlohmann::json wifiArray = nlohmann::json::array();

    for (const auto &adapter : adapters) {
      nlohmann::json adapterObj;
      adapterObj["name"] = adapter.GetName();
      adapterObj["mac_address"] = adapter.GetMacAddress();
      adapterObj["ip_address"] = adapter.GetIPAddress();
      adapterObj["status"] = adapter.GetStatus();

      if (adapter.IsEthernet()) {
        ethernetArray.push_back(adapterObj);
      } else if (adapter.IsWiFi()) {
        wifiArray.push_back(adapterObj);
      }
    }

    networkObj["ethernet"] = ethernetArray;
    networkObj["wifi"] = wifiArray;
    root["network"] = networkObj;
  } catch (...) {
  }
}

static void AppendAudioInfo(nlohmann::json &root, const nysys::AudioList *audioList) noexcept {
  if (!audioList) {
    return;
  }

  try {
    const auto &devices = audioList->GetDevices();
    nlohmann::json audioArray = nlohmann::json::array();

    for (const auto &device : devices) {
      nlohmann::json deviceObj;
      deviceObj["name"] = device.GetName();
      deviceObj["manufacturer"] = device.GetManufacturer();
      audioArray.push_back(deviceObj);
    }

    root["audio"] = audioArray;
  } catch (...) {
  }
}

static void AppendBatteryInfo(nlohmann::json &root, const nysys::BatteryInfo *batteryInfo) noexcept {
  if (!batteryInfo) {
    return;
  }

  try {
    nlohmann::json batteryObj;
    batteryObj["is_desktop"] = batteryInfo->IsDesktop();
    batteryObj["percent"] = batteryInfo->GetPercent();
    batteryObj["power_plugged"] = batteryInfo->IsPluggedIn();

    root["battery"] = batteryObj;
  } catch (...) {
  }
}

static void AppendMonitorInfo(nlohmann::json &root, const nysys::MonitorList *monitorList) noexcept {
  if (!monitorList) {
    return;
  }

  try {
    const auto &monitors = monitorList->GetMonitors();
    nlohmann::json monitorArray = nlohmann::json::array();

    for (const auto &monitor : monitors) {
      nlohmann::json monitorObj;
      monitorObj["is_primary"] = monitor.IsPrimary();
      monitorObj["width"] = monitor.GetWidth();
      monitorObj["height"] = monitor.GetHeight();
      monitorObj["current_resolution"] = monitor.GetCurrentResolution();
      monitorObj["native_resolution"] = monitor.GetNativeResolution();
      monitorObj["aspect_ratio"] = monitor.GetAspectRatio();
      monitorObj["refresh_rate"] = monitor.GetRefreshRate();
      monitorObj["screen_size"] = monitor.GetScreenSize();
      monitorObj["physical_width_mm"] = monitor.GetPhysicalWidthMm();
      monitorObj["physical_height_mm"] = monitor.GetPhysicalHeightMm();
      monitorObj["manufacturer"] = monitor.GetManufacturer();
      monitorObj["device_id"] = monitor.GetDeviceId();
      monitorArray.push_back(monitorObj);
    }

    root["monitors"] = monitorArray;
  } catch (...) {
  }
}

std::optional<std::string> GenerateSystemInfo(const nysys::GPUList *gpuList, const nysys::MotherboardInfo *mbInfo,
                                              const nysys::CPUList *cpuList, const nysys::MemoryInfo *memInfo,
                                              const nysys::StorageList *storageList,
                                              const nysys::NetworkList *networkList, const nysys::AudioList *audioList,
                                              const nysys::BatteryInfo *batteryInfo,
                                              const nysys::MonitorList *monitorList,
                                              const JsonConfig &config) noexcept {
  try {
    nlohmann::json root;

    AppendGpuInfo(root, gpuList);
    AppendMotherboardInfo(root, mbInfo);
    AppendCpuInfo(root, cpuList);
    AppendMemoryInfo(root, memInfo);
    AppendStorageInfo(root, storageList);
    AppendNetworkInfo(root, networkList);
    AppendAudioInfo(root, audioList);
    AppendBatteryInfo(root, batteryInfo);
    AppendMonitorInfo(root, monitorList);

    if (config.prettyPrint) {
      return root.dump(config.indentSize);
    } else {
      return root.dump();
    }
  } catch (...) {
    return std::nullopt;
  }
}

}  // namespace json