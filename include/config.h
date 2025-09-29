#ifndef CONFIG_H_
#define CONFIG_H_

#include <chrono>
#include <cstdint>
#include <cstdlib>

namespace config {
  // ===========================================================================
  // TIMING CONFIGURATION
  // ===========================================================================
  constexpr uint16_t kSampleRate {250};  // Samples per second (SPS)
  constexpr double kPeriodSeconds {1.0 / kSampleRate};  // 0.004 seconds
  constexpr int kPeriodUs {static_cast<int>(kPeriodSeconds * 1'000'000)};  // 4000 μs
  constexpr auto kSamplePeriod {std::chrono::microseconds(kPeriodUs)};
  
  // ===========================================================================
  // ACQUISITION CONFIGURATION
  // ===========================================================================
  constexpr std::chrono::seconds kAcquisitionDuration {70};
  
  // ===========================================================================
  // BUFFER CONFIGURATION
  // ===========================================================================
  constexpr size_t kBufferSize = kSampleRate * kAcquisitionDuration.count();
  
  // ===========================================================================
  // FILE MANAGEMENT CONFIGURATION
  // ===========================================================================
  constexpr auto kFileWriteInterval = std::chrono::milliseconds(200);
  
  // ===========================================================================
  // LOGGING CONFIGURATION
  // ===========================================================================
  constexpr const char* kDefaultLogFile = "cardiac_system.log";
  
}

#endif
