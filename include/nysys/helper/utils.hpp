#ifndef UTILS_HPP
#define UTILS_HPP

#include <cmath>
#include <cstdint>

namespace utils {

constexpr double kBytesPerGigabyte = 1024.0 * 1024.0 * 1024.0;

inline double RoundToDecimalPlaces(double value, int places = 2) {
  const double multiplier = std::pow(10.0, places);
  return std::round(value * multiplier) / multiplier;
}

inline double BytesToGB(uint64_t bytes) { return RoundToDecimalPlaces(static_cast<double>(bytes) / kBytesPerGigabyte); }

inline double DoubleToGB(double bytes) { return RoundToDecimalPlaces(bytes / kBytesPerGigabyte); }

}  // namespace utils

#endif