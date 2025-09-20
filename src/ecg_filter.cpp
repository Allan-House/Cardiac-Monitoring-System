#include "ecg_filter.h"

void ECGFilter::Initialize(double x0) {
  for (size_t sec = 0; sec < kSections.size(); ++sec) {
    state_[sec][0] = kZi[sec][0] * x0;
    state_[sec][1] = kZi[sec][1] * x0;
  }
}

float ECGFilter::Process(float input) {
  double y = static_cast<double>(input);
  
  for (size_t sec = 0; sec < kSections.size(); ++sec) {
    const auto& s = kSections[sec];
    double& z1 = state_[sec][0];
    double& z2 = state_[sec][1];

    // Direct Form II Transposed
    double out = s.b0 * y + z1;
    z1 = s.b1 * y - s.a1 * out + z2;
    z2 = s.b2 * y - s.a2 * out;
    y = out;
  }
  
  return static_cast<float>(y);
}

void ECGFilter::Reset() {
  for (auto& st : state_) {
    st[0] = st[1] = 0.0;
  }
}