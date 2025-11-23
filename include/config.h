#ifndef CONFIG_H_
#define CONFIG_H_

#include <chrono>
#include <cstdint>
#include <cstdlib>

namespace config {
  
  constexpr float kVoltageRange {4.096f};

  static_assert(kVoltageRange == 6.144f || kVoltageRange == 4.096f ||
                kVoltageRange == 2.048f || kVoltageRange == 1.024f ||
                kVoltageRange == 0.512f || kVoltageRange == 0.256f,
                "[ERROR] - Voltage range must match ADS1115 options.");
  
  constexpr uint16_t kSampleRate {475};  // Samples per second (SPS)

  static_assert(kSampleRate == 8   || kSampleRate == 16  ||
                kSampleRate == 32  || kSampleRate == 64  ||
                kSampleRate == 128 || kSampleRate == 250 ||
                kSampleRate == 475 || kSampleRate == 860,
                "[ERROR] - Sample rate must match ADS1115 options.");

  constexpr double kPeriodSeconds {1.0 / kSampleRate};
  constexpr int kPeriodUs {static_cast<int>(kPeriodSeconds * 1'000'000)};
  constexpr auto kSamplePeriod {std::chrono::microseconds(kPeriodUs)};
  
  constexpr std::chrono::seconds kAcquisitionDuration {60};
  
  constexpr size_t kBufferSize {kSampleRate * kAcquisitionDuration.count()};
  
  constexpr auto kFileWriteInterval {std::chrono::milliseconds(200)};
  
  constexpr const char* kDefaultLogFile {"system.log"};
  
  // Notch filter configuration
  constexpr bool kEnableNotchFilter {true};          // Enable/disable notch filtering
  constexpr float kNotchCenterFreq {60.0f};          // Center frequency (Hz) - power line interference
  constexpr float kNotchQFactor {30.0f};             // Quality factor (Q = f0/BW)
  
  static_assert(kNotchCenterFreq > 0.0f && kNotchCenterFreq < (kSampleRate / 2.0f),
                "[ERROR] - Notch center frequency must be between 0 and Nyquist frequency.");
  
  static_assert(kNotchQFactor > 0.0f,
                "[ERROR] - Notch Q factor must be positive.");
  
}

#endif
