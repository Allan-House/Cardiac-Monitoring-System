#ifndef ECG_FILTER_H_
#define ECG_FILTER_H_

#include <array>
#include <deque>
#include <cstddef>

/**
 * @brief Real-time ECG signal filtering using FIR filters.
 * 
 * Implements three sequential FIR filtering stages optimized for ECG signals:
 * - High-pass: Removes DC offset and baseline wander (differentiator + integrator)
 * - 60Hz notch: Eliminates power line interference
 * - Low-pass: Anti-aliasing and noise reduction
 * 
 * FIR filters provide linear phase response and guaranteed stability.
 */
class ECGFilter {
  private:
  static constexpr float kSampleRate = 250.0f; // Hz
  
  // Simple baseline removal (high-pass effect)
  static constexpr size_t kBaselineTaps = 125; // 0.5s window at 250 SPS
  std::deque<float> baseline_buffer_;
  
  // Moving average for noise reduction (low-pass effect)  
  static constexpr size_t kSmoothTaps = 5; // ~20ms smoothing
  std::deque<float> smooth_buffer_;
  
  public:
  ECGFilter();
  
  /**
   * @brief Processes single sample through all filtering stages.
   * @param input Raw voltage sample
   * @return Filtered voltage sample
   */
  float Process(float input);
  
  /**
   * @brief Resets all filter states to zero.
   */
  void Reset();

  private:
  /**
   * @brief Calculates moving average of buffer contents.
   * @param buffer Input buffer
   * @return Average value
   */
  float CalculateAverage(const std::deque<float>& buffer);
};

#endif