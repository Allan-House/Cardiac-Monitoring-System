#ifndef NOTCH_FILTER_H_
#define NOTCH_FILTER_H_

#include <cmath>

/**
 * @brief Digital notch filter for power line interference rejection.
 * 
 * Implements a second-order IIR (biquad) notch filter using Direct Form II
 * Transposed structure for numerical stability. Designed to attenuate
 * 60 Hz power line interference in ECG signals while preserving the
 * diagnostic frequency content (0.5-40 Hz).
 * 
 * Filter characteristics:
 * - Type: IIR Notch (Band-Stop)
 * - Order: 2nd order (biquad)
 * - Center frequency: Configurable (default: 60 Hz)
 * - Quality factor: Configurable (default: 30)
 * - Bandwidth: f₀/Q (e.g., 60/30 = 2 Hz)
 * - Attenuation at f₀: ~40 dB for Q=30
 * 
 * Implementation uses RBJ Audio EQ Cookbook equations for coefficient
 * calculation, ensuring accurate frequency response and stability.
 */
class NotchFilter {
  private:
  // Filter coefficients (numerator)
  float b0_;
  float b1_;
  float b2_;
  
  // Filter coefficients (denominator, a0 normalized to 1)
  float a1_;
  float a2_;
  
  // Filter state variables (Direct Form II Transposed)
  float w1_;  // First delay element
  float w2_;  // Second delay element
  
  // Filter parameters
  float center_freq_;   // Center frequency (Hz)
  float sample_rate_;   // Sample rate (Hz)
  float Q_;             // Quality factor
  
  bool initialized_;
  
  public:
  /**
   * @brief Constructs notch filter with specified parameters.
   * 
   * @param center_freq Center frequency to reject (Hz)
   * @param sample_rate System sample rate (Hz)
   * @param Q Quality factor (bandwidth = center_freq/Q)
   * 
   * @note Higher Q values provide narrower rejection but require
   *       more precise frequency targeting. Q=30 is recommended
   *       for 60 Hz rejection in ECG applications.
   */
  NotchFilter(float center_freq, float sample_rate, float Q);
  
  /**
   * @brief Initializes filter by calculating coefficients.
   * 
   * Computes biquad coefficients using RBJ Audio EQ Cookbook formulas.
   * Must be called before processing samples.
   * 
   * @return true if initialization successful, false on error
   *         (e.g., invalid parameters, Nyquist violation)
   */
  bool Init();
  
  /**
   * @brief Processes single sample through notch filter.
   * 
   * Applies Direct Form II Transposed biquad structure:
   * 
   *   y[n] = b0*x[n] + w1[n-1]
   *   w1[n] = b1*x[n] - a1*y[n] + w2[n-1]
   *   w2[n] = b2*x[n] - a2*y[n]
   * 
   * @param input Input sample
   * @return Filtered output sample
   * 
   * @note This is a real-time operation with O(1) complexity
   */
  float Process(float input);
  
  /**
   * @brief Resets filter state to zero.
   * 
   * Clears internal delay elements. Use when discontinuity
   * in input signal occurs or when restarting filtering.
   */
  void Reset();
  
  /**
   * @brief Checks if filter is initialized and ready.
   * 
   * @return true if Init() was called successfully, false otherwise
   */
  bool IsInitialized() const { return initialized_; }
  
  // Getters for filter parameters
  float get_center_freq() const { return center_freq_; }
  float get_sample_rate() const { return sample_rate_; }
  float get_Q() const { return Q_; }
  float get_bandwidth() const { return center_freq_ / Q_; }
  
  private:
  /**
   * @brief Calculates biquad filter coefficients.
   * 
   * Uses RBJ Audio EQ Cookbook notch filter equations:
   * 
   *   ω₀ = 2π * f₀ / fs
   *   α = sin(ω₀) / (2*Q)
   *   
   *   b0 = 1
   *   b1 = -2 * cos(ω₀)
   *   b2 = 1
   *   
   *   a0 = 1 + α
   *   a1 = -2 * cos(ω₀)
   *   a2 = 1 - α
   *   
   * All coefficients normalized by a0.
   */
  void CalculateCoefficients();
  
  /**
   * @brief Validates filter parameters.
   * 
   * Checks:
   * - Center frequency < Nyquist frequency
   * - Sample rate > 0
   * - Q > 0
   * 
   * @return true if parameters valid, false otherwise
   */
  bool ValidateParameters() const;
};

#endif
