#ifndef JSON_STRUCTURE_HPP
#define JSON_STRUCTURE_HPP

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace nysys {

class GPUList;
class MotherboardInfo;
class CPUList;
class MemoryInfo;
class StorageList;
class NetworkList;
class AudioList;
class BatteryInfo;
class MonitorList;
}  // namespace nysys

namespace json {

struct JsonConfig {
  bool prettyPrint = true;
  int indentSize = 2;

  [[nodiscard]] static constexpr JsonConfig Default() noexcept { return JsonConfig{}; }
};

[[nodiscard]] std::optional<std::string> GenerateSystemInfo(
    const nysys::GPUList *gpuList, const nysys::MotherboardInfo *mbInfo, const nysys::CPUList *cpuList,
    const nysys::MemoryInfo *memInfo, const nysys::StorageList *storageList, const nysys::NetworkList *networkList,
    const nysys::AudioList *audioList, const nysys::BatteryInfo *batteryInfo, const nysys::MonitorList *monitorList,
    const JsonConfig &config = JsonConfig::Default()) noexcept;

}  // namespace json

#endif