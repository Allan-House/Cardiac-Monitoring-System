#ifndef ECG_FILTER_H_
#define ECG_FILTER_H_

#include <array>
#include <cstddef>

/**
 * @brief Real-time ECG signal filtering using cascaded second-order sections.
 * 
 * Implements two-stage filtering optimized for ECG signals:
 * - 0.67Hz high-pass (Butterworth) - removes DC offset and baseline wander
 * - 45Hz low-pass (Butterworth, 4th-order) - anti-aliasing and noise reduction
 */
class ECGFilter {
  private:
  struct Section {
    double b0, b1, b2;
    double a0, a1, a2; // a0 assumed = 1.0
  };

  // Cascade: High-pass → Low-pass
  static constexpr std::array<Section, 3> kSections {{
    // High-pass 0.67 Hz (Butterworth, order=2)
    { 9.887320411951481036e-01, -1.977464082390296207e+00, 9.887320411951481036e-01,
      1.000000000000000000e+00, -1.977389851896891936e+00, 9.774888239071933170e-01 },

    // Low-pass 45 Hz (Butterworth, order=4) - section 0
    { 7.820803718526770399e-02, 1.564160743705354080e-01, 7.820803718526770399e-02,
      1.000000000000000000e+00, -6.436177662670825659e-01, 1.416998273329224591e-01 },

    // Low-pass 45 Hz (Butterworth, order=4) - section 1
    { 1.000000000000000000e+00, 2.000000000000000000e+00, 1.000000000000000000e+00,
      1.000000000000000000e+00, -4.128015980042388954e-01, 3.621365270750983486e-01 }
  }};

  // Initial conditions for each SOS
  static constexpr std::array<std::array<double, 2>, 3> kZi {{
    { -9.887320411951e-01,  9.887320411951e-01 },  // HP
    { -7.820803718527e-02,  0.000000000000e+00 },  // LP section 0
    { -1.000000000000e+00,  0.000000000000e+00 },  // LP section 1
  }};

  // Runtime state [section][z1, z2]
  std::array<std::array<double, 2>, 3> state_;

  public:
  ECGFilter() { Reset(); }

  /**
   * @brief Initialize filter states with first sample (avoids startup transient).
   * @param x0 First input sample value
   */
  void Initialize(double x0);

  /**
   * @brief Process one sample through the filter cascade.
   * @param input Raw voltage sample
   * @return Filtered voltage sample
   */
  float Process(float input);

  /**
   * @brief Reset all filter states to zero.
   */
  void Reset();
};

#endif