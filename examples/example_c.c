#include <windows.h>

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nysys.h"

static int g_updateCount = 0;
static clock_t g_startTime = 0;

void callbackFunction(const char *jsonData) {
  if (jsonData == NULL) {
    printf("\nError: Received NULL data in callback\n");
    return;
  }

  g_updateCount++;

  double elapsed = (double)(clock() - g_startTime) / CLOCKS_PER_SEC;

  printf("\rUpdate #%d (%.1fs) - %zu bytes", g_updateCount, elapsed, strlen(jsonData));

  _mkdir("output");

  char filename[100];
  sprintf(filename, "output/system_info_%d.json", g_updateCount);

  FILE *file = fopen(filename, "w");
  if (file != NULL) {
    fputs(jsonData, file);
    fclose(file);
    printf(" - Saved to %s", filename);
    fflush(stdout);
  } else {
    printf(" - Error saving to %s", filename);
  }
}

int main(void) {
  printf("NySys C Example - System Info Collector\n");
  printf("API Version: NySys v0.5.0beta\n\n");

  g_startTime = clock();

  set_callback(callbackFunction);
  printf("Callback registered successfully.\n");

  int32_t updateInterval = NYSYS_DEFAULT_UPDATE_INTERVAL_MS;
  printf("Using update interval: %d ms\n", updateInterval);

  if (!start_monitoring(updateInterval)) {
    printf("Failed to start monitoring!\n");
    printf("This could be due to insufficient permissions or system resources.\n");
    return 1;
  }

  printf("Monitoring started successfully.\n");
  printf("Press Enter to stop monitoring...\n\n");

  getchar();

  printf("\nStopping monitoring...\n");
  stop_monitoring();

  double totalTime = (double)(clock() - g_startTime) / CLOCKS_PER_SEC;

  printf("Monitoring Summary:\n");
  printf("- Total updates received: %d\n", g_updateCount);
  printf("- Total runtime: %.1f seconds\n", totalTime);
  if (g_updateCount > 0) {
    printf("- Average update rate: %.1f updates/sec\n", g_updateCount / totalTime);
  }
  printf("- JSON files saved in 'output' directory\n");
  printf("\nMonitoring completed successfully.\n");

  return 0;
}