#ifndef NYSYS_HPP
#define NYSYS_HPP

#include <windows.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

#include "internal.hpp"

#ifdef NYSYS_EXPORTS
#define NYSYS_API __declspec(dllexport)
#else
#define NYSYS_API __declspec(dllimport)
#endif

namespace nysys {
constexpr int32_t MIN_UPDATE_INTERVAL_MS = 100;
constexpr int32_t DEFAULT_UPDATE_INTERVAL_MS = 1000;
constexpr int32_t MAX_THREAD_WAIT_MS = 5000;
}  // namespace nysys

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*NysysCallback)(const char *jsonData);

NYSYS_API BOOL start_monitoring(int32_t updateIntervalMs);
NYSYS_API void stop_monitoring(void);
NYSYS_API void set_update_interval(int32_t updateIntervalMs);
NYSYS_API void set_callback(NysysCallback callback);
NYSYS_API BOOL is_monitoring(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace nysys {

NYSYS_API bool StartMonitoring(int32_t updateIntervalMs = DEFAULT_UPDATE_INTERVAL_MS);
NYSYS_API void StopMonitoring() noexcept;
NYSYS_API void SetUpdateInterval(int32_t updateIntervalMs);
NYSYS_API void SetCallback(const std::function<void(const std::string &)> &callback);
NYSYS_API bool IsMonitoring() noexcept;
NYSYS_API MonitoringError GetLastError() noexcept;
NYSYS_API std::chrono::milliseconds GetUptime() noexcept;

}  // namespace nysys
#endif

#endif