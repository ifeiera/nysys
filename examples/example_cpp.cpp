#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "nysys.hpp"

std::string formatDuration(std::chrono::milliseconds ms) {
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(ms);
  auto remaining_ms = ms - seconds;

  std::ostringstream oss;
  oss << seconds.count() << "." << std::setfill('0') << std::setw(3) << remaining_ms.count() << "s";
  return oss.str();
}

std::string getErrorDescription(nysys::MonitoringError error) {
  switch (error) {
    case nysys::MonitoringError::Success:
      return "Success";
    case nysys::MonitoringError::InvalidParameter:
      return "Invalid parameter";
    case nysys::MonitoringError::AlreadyRunning:
      return "Already running";
    case nysys::MonitoringError::NotRunning:
      return "Not running";
    case nysys::MonitoringError::ThreadCreationFailed:
      return "Thread creation failed";
    case nysys::MonitoringError::ThreadTerminationFailed:
      return "Thread termination failed";
    case nysys::MonitoringError::SystemResourceError:
      return "System resource error";
    case nysys::MonitoringError::DataCollectionFailed:
      return "Data collection failed";
    case nysys::MonitoringError::JsonGenerationFailed:
      return "JSON generation failed";
    case nysys::MonitoringError::CallbackFailed:
      return "Callback failed";
    case nysys::MonitoringError::CallbackExecutionFailed:
      return "Callback execution failed";
    case nysys::MonitoringError::UnknownError:
      return "Unknown error";
    default:
      return "Undefined error";
  }
}

int main() {
  std::cout << "NySys CPP Example - System Info Collector\n";
  std::cout << "API Version: NySys v0.5.0beta\n\n";

  std::atomic<int> updateCount{0};
  auto startTime = std::chrono::steady_clock::now();

  try {
    std::filesystem::create_directories("output");
    std::cout << "Output directory created/verified.\n";

    nysys::SetCallback([&updateCount, startTime](const std::string &jsonData) {
      if (jsonData.empty()) {
        std::cerr << "\nWarning: Received empty JSON data\n";
        return;
      }

      int currentCount = ++updateCount;
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);

      std::cout << "\rUpdate #" << currentCount << " (" << formatDuration(elapsed) << ") - " << jsonData.size()
                << " bytes";

      try {
        std::string filename = "output/system_info_" + std::to_string(currentCount) + ".json";
        std::ofstream file(filename);
        if (file.is_open()) {
          file << jsonData;
          file.close();
          std::cout << " - Saved to " << filename;
        } else {
          std::cout << " - Error: Could not save to " << filename;
        }
      } catch (const std::exception &e) {
        std::cout << " - File error: " << e.what();
      }

      std::cout.flush();
    });

    std::cout << "Callback registered successfully.\n";

    int32_t updateInterval = nysys::DEFAULT_UPDATE_INTERVAL_MS;
    std::cout << "Using update interval: " << updateInterval << " ms\n";

    std::cout << "Starting monitoring...\n";
    bool started = nysys::StartMonitoring(updateInterval);

    if (!started) {
      auto lastError = nysys::GetLastError();
      std::cerr << "Failed to start monitoring!\n";
      std::cerr << "Error: " << getErrorDescription(lastError) << "\n";
      std::cerr << "This could be due to insufficient permissions or system "
                   "resources.\n";
      return 1;
    }

    std::cout << "Monitoring started successfully.\n";
    std::cout << "Press Enter to stop monitoring...\n\n";

    std::cin.get();

    auto uptime = nysys::GetUptime();
    auto lastError = nysys::GetLastError();

    std::cout << "\nStopping monitoring...\n";
    nysys::StopMonitoring();

    auto endTime = std::chrono::steady_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "Monitoring Summary:\n";
    std::cout << "- Total updates received: " << updateCount.load() << "\n";
    std::cout << "- Total runtime: " << formatDuration(totalTime) << "\n";
    std::cout << "- Monitoring uptime: " << formatDuration(uptime) << "\n";

    if (updateCount > 0 && totalTime.count() > 0) {
      double rate = static_cast<double>(updateCount.load()) / (totalTime.count() / 1000.0);
      std::cout << "- Average update rate: " << std::fixed << std::setprecision(1) << rate << " updates/sec\n";
    }

    std::cout << "- Final status: " << getErrorDescription(lastError) << "\n";
    std::cout << "- JSON files saved in 'output' directory\n";
    std::cout << "\nMonitoring completed successfully.\n";
  } catch (const nysys::MonitoringException &e) {
    std::cerr << "\nMonitoring Exception: " << e.what() << "\n";
    std::cerr << "Error Code: " << getErrorDescription(e.GetErrorCode()) << "\n";
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "\nStandard Exception: " << e.what() << "\n";
    return 1;
  } catch (...) {
    std::cerr << "\nUnknown exception occurred!\n";
    return 1;
  }

  return 0;
}