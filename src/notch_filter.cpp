#include "notch_filter.h"

#include <cmath>

#include "logger.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


NotchFilter::NotchFilter(float center_freq, float sample_rate, float Q)
  : b0_ {0.0f},
    b1_ {0.0f},
    b2_ {0.0f},
    a1_ {0.0f},
    a2_ {0.0f},
    w1_ {0.0f},
    w2_ {0.0f},
    center_freq_ {center_freq},
    sample_rate_ {sample_rate},
    Q_ {Q},
    initialized_ {false}
{
  // Constructor - coefficients calculated in Init()
}


bool NotchFilter::Init() {
  if (!ValidateParameters()) {
    LOG_ERROR("Invalid notch filter parameters: f0=%.1f Hz, fs=%.1f Hz, Q=%.1f",
              center_freq_, sample_rate_, Q_);
    return false;
  }
  
  CalculateCoefficients();
  Reset();
  
  initialized_ = true;
  
  LOG_SUCCESS("Notch filter initialized: f0=%.1f Hz, Q=%.1f, BW=%.2f Hz",
              center_freq_, Q_, get_bandwidth());
  
  return true;
}


void NotchFilter::CalculateCoefficients() {
  // Angular frequency (radians/sample)
  float omega0 = 2.0f * M_PI * center_freq_ / sample_rate_;
  
  // Bandwidth parameter
  float alpha = std::sin(omega0) / (2.0f * Q_);
  
  // Cosine of center frequency (used in multiple coefficients)
  float cos_omega0 = std::cos(omega0);
  
  // Numerator coefficients (before normalization)
  float b0_raw = 1.0f;
  float b1_raw = -2.0f * cos_omega0;
  float b2_raw = 1.0f;
  
  // Denominator coefficients (before normalization)
  float a0_raw = 1.0f + alpha;
  float a1_raw = -2.0f * cos_omega0;
  float a2_raw = 1.0f - alpha;
  
  // Normalize by a0
  b0_ = b0_raw / a0_raw;
  b1_ = b1_raw / a0_raw;
  b2_ = b2_raw / a0_raw;
  
  // a0 becomes 1 after normalization (implicit)
  a1_ = a1_raw / a0_raw;
  a2_ = a2_raw / a0_raw;
  
  #ifdef DEBUG
  LOG_DEBUG("Notch filter coefficients calculated:");
  LOG_DEBUG("  b0=%.6f, b1=%.6f, b2=%.6f", b0_, b1_, b2_);
  LOG_DEBUG("  a1=%.6f, a2=%.6f", a1_, a2_);
  #endif
}


bool NotchFilter::ValidateParameters() const {
  // Check sample rate
  if (sample_rate_ <= 0.0f) {
    LOG_ERROR("Sample rate must be positive: %.1f Hz", sample_rate_);
    return false;
  }
  
  // Check Nyquist criterion
  float nyquist_freq = sample_rate_ / 2.0f;
  if (center_freq_ <= 0.0f || center_freq_ >= nyquist_freq) {
    LOG_ERROR("Center frequency (%.1f Hz) must be between 0 and Nyquist (%.1f Hz)",
              center_freq_, nyquist_freq);
    return false;
  }
  
  // Check Q factor
  if (Q_ <= 0.0f) {
    LOG_ERROR("Q factor must be positive: %.1f", Q_);
    return false;
  }
  
  // Warn if Q is very high (potential numerical issues)
  if (Q_ > 100.0f) {
    LOG_WARN("Very high Q factor (%.1f) may cause numerical instability", Q_);
  }
  
  return true;
}


float NotchFilter::Process(float input) {
  if (!initialized_) {
    LOG_ERROR("Notch filter not initialized - call Init() first");
    return input;  // Pass-through if not initialized
  }
  
  // Direct Form II Transposed implementation
  // This structure provides better numerical properties than Direct Form I
  
  // Calculate output
  float output = b0_ * input + w1_;
  
  // Update state variables
  float w1_new = b1_ * input - a1_ * output + w2_;
  float w2_new = b2_ * input - a2_ * output;
  
  w1_ = w1_new;
  w2_ = w2_new;
  
  return output;
}


void NotchFilter::Reset() {
  w1_ = 0.0f;
  w2_ = 0.0f;
  
  #ifdef DEBUG
  LOG_DEBUG("Notch filter state reset");
  #endif
}
