#ifndef CONFIG_H_
#define CONFIG_H_

#include <chrono>
#include <cstdint>
#include <cstdlib>

namespace config {
  // Timing configuration
  constexpr uint16_t kSampleRate {250};  // Samples per second (SPS)

  static_assert(kSampleRate == 8   || kSampleRate == 16  ||
                kSampleRate == 32  || kSampleRate == 64  ||
                kSampleRate == 128 || kSampleRate == 250 ||
                kSampleRate == 475 || kSampleRate == 860,
                "[ERROR] - Sample rate must match ADS1115 options.");

  constexpr double kPeriodSeconds {1.0 / kSampleRate};  // 0.004 seconds
  constexpr int kPeriodUs {static_cast<int>(kPeriodSeconds * 1'000'000)};  // 4000 μs
  constexpr auto kSamplePeriod {std::chrono::microseconds(kPeriodUs)};
  
  // Acquisition configuration
  constexpr std::chrono::seconds kAcquisitionDuration {70};
  
  // Buffer configuration 
  constexpr size_t kBufferSize = kSampleRate * kAcquisitionDuration.count();
  
  // File management configuration
  constexpr auto kFileWriteInterval = std::chrono::milliseconds(200);
  
  // Logging configuration
  constexpr const char* kDefaultLogFile = "cardiac_system.log";
  
}

#endif
