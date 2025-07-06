#ifndef NYSYS_H
#define NYSYS_H

#include <windows.h>

#include <stdint.h>

#ifdef NYSYS_EXPORTS
#define NYSYS_API __declspec(dllexport)
#else
#define NYSYS_API __declspec(dllimport)
#endif

#define NYSYS_MIN_UPDATE_INTERVAL_MS 100
#define NYSYS_DEFAULT_UPDATE_INTERVAL_MS 1000
#define NYSYS_MAX_THREAD_WAIT_MS 5000

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

#endif