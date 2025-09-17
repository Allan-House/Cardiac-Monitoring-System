#include "ecg_filter.h"

ECGFilter::ECGFilter() {
  // Initialize buffers with zeros
  baseline_buffer_.resize(kBaselineTaps, 0.0f);
  smooth_buffer_.resize(kSmoothTaps, 0.0f);
}

float ECGFilter::Process(float input) {
  // Stage 1: Baseline removal (subtract moving average for high-pass effect)
  baseline_buffer_.push_front(input);
  if (baseline_buffer_.size() > kBaselineTaps) {
    baseline_buffer_.pop_back();
  }
  
  float baseline_avg = 0.0f;
  for (float val : baseline_buffer_) {
    baseline_avg += val;
  }
  baseline_avg /= baseline_buffer_.size();
  
  float baseline_removed = input - baseline_avg;
  
  // Stage 2: Simple smoothing (moving average for noise reduction)
  smooth_buffer_.push_front(baseline_removed);
  if (smooth_buffer_.size() > kSmoothTaps) {
    smooth_buffer_.pop_back();
  }
  
  float smoothed = 0.0f;
  for (float val : smooth_buffer_) {
    smoothed += val;
  }
  smoothed /= smooth_buffer_.size();
  
  return smoothed;
}

void ECGFilter::Reset() {
  std::fill(baseline_buffer_.begin(), baseline_buffer_.end(), 0.0f);
  std::fill(smooth_buffer_.begin(), smooth_buffer_.end(), 0.0f);
}

float ECGFilter::CalculateAverage(const std::deque<float>& buffer) {
  if (buffer.empty()) return 0.0f;
  
  float sum = 0.0f;
  for (float val : buffer) {
    sum += val;
  }
  return sum / buffer.size();
}