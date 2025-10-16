#include "ecg_analyzer.h"

#include <cstdint>

#include "logger.h"

ECGAnalyzer::ECGAnalyzer(std::shared_ptr<RingBuffer<Sample>> buffer_raw,
                         std::shared_ptr<RingBuffer<Sample>> buffer_classified)
  : buffer_raw_ {buffer_raw},
    buffer_classified_ {buffer_classified}
{
  // Reserve space to avoid reallocations during execution
  samples_.reserve(config::kSampleRate * 2); // 2 seconds at configured rate 
  detected_beats_.reserve(5);   // // ~2-3 beats max in 2-second window
}


void ECGAnalyzer::Run() {
  processing_ = true;
  processing_thread_ = std::thread(&ECGAnalyzer::ProcessingLoop, this);
}


void ECGAnalyzer::Stop() {
  LOG_INFO("Stopping ECG processing thread...");

  processing_ = false;
  buffer_raw_->Shutdown();

  if (processing_thread_.joinable()) {
    processing_thread_.join();
  }

  LOG_INFO("ECG processing stopped");
}


void ECGAnalyzer::ProcessingLoop() {
  LOG_INFO("Starting ECG processing thread...");

  #ifdef DEBUG
  uint32_t sample_count {0};
  #endif

  while (processing_) {
    auto data {buffer_raw_->Consume()};
    
    if (!data) {
      LOG_INFO("Processing interrupted - buffer shutdown");
      break;
    }
    
    ProcessSample(data.value());

    #ifdef DEBUG
    sample_count++;
    if (sample_count % 1000 == 0) {
      LOG_DEBUG("Processed %u samples, detected %zu beats", 
                sample_count, detected_beats_.size());
    }
    #endif
  }

  LOG_INFO("Processing remaining samples in buffer...");
  
  // Process remaining samples
  auto data {buffer_raw_->Consume()};
  while (data) {
    ProcessSample(data.value());
    data = buffer_raw_->Consume();
  }

  // Final processing
  ProcessFinalData();
  
  buffer_classified_->Shutdown();
  LOG_INFO("Processing thread finished.");
}


void ECGAnalyzer::ProcessSample(const Sample& sample) {
  samples_.push_back(sample);
  
  if (samples_.size() >= 3) {
    DetectRPeaks();
  }
  
  ProcessCompleteBeats();
  
  TransferProcessedSamples();
}


void ECGAnalyzer::DetectRPeaks() {
  // Check the sample before current (1 sample delay for reliable peak detection)
  size_t check_position {samples_.size() - 2};
  
  if (IsRPeak(check_position)) {
    detected_beats_.emplace_back(check_position);
    LOG_DEBUG("R peak detected at position %zu", check_position);
  }
}


bool ECGAnalyzer::IsRPeak(size_t pos) const {
  if (pos == 0 || pos >= samples_.size() - 1) {
    return false;
  }
  
  const Sample& prev {samples_.at(pos - 1)};
  const Sample& curr {samples_.at(pos)};
  const Sample& next {samples_.at(pos + 1)};
  
  // Peak detection: higher than neighbors and above threshold
  bool is_peak {(curr.voltage > prev.voltage) && 
                (curr.voltage > next.voltage) && 
                (curr.voltage > ecg_config::kRThreshold)};
  
  // Refractory period: avoid detecting peaks too close together
  if (is_peak && !detected_beats_.empty()) {
    size_t last_r {detected_beats_.back().r_pos};
    if (pos - last_r < ecg_config::kRefractoryPeriod) {
      return false;
    }
  }
  
  return is_peak;
}


void ECGAnalyzer::ProcessCompleteBeats() {
  for (auto& beat : detected_beats_) {
    
    // Detect QRS complex (Q and S points)
    if (!beat.qrs_complete && CanCompleteQRS(beat.r_pos)) {
      // Q point: lowest signal value in 80ms before R
      size_t q_start {beat.r_pos - ecg_config::kQSWindow};
      beat.q_pos = FindLowest(q_start, beat.r_pos);
      
      // S point: lowest signal value in 80ms after R
      size_t s_end {beat.r_pos + ecg_config::kQSWindow};
      beat.s_pos = FindLowest(beat.r_pos + 1, s_end);
      
      beat.qrs_complete = true;
      LOG_DEBUG("QRS complex completed for beat at %zu", beat.r_pos);
    }
    
    // Detect P wave (highest in 200ms before Q)
    if (beat.qrs_complete && !beat.p_complete && CanCompleteP(beat.q_pos)) {
      size_t p_start {beat.q_pos - ecg_config::kPWindow};
      beat.p_pos = FindHighest(p_start, beat.q_pos);
      beat.p_complete = true;
      LOG_DEBUG("P wave completed for beat at %zu", beat.r_pos);
    }
    
    // Detect T wave (highest in 400ms after S)
    if (beat.qrs_complete && !beat.t_complete && CanCompleteT(beat.s_pos)) {
      size_t t_end {beat.s_pos + ecg_config::kTWindow};
      beat.t_pos = FindHighest(beat.s_pos + 1, t_end);
      beat.t_complete = true;
      LOG_DEBUG("T wave completed for beat at %zu", beat.r_pos);
    }
  }
}


bool ECGAnalyzer::CanCompleteQRS(size_t r_pos) const {
  return (r_pos >= ecg_config::kQSWindow) && 
         (r_pos + ecg_config::kQSWindow < samples_.size());
}


bool ECGAnalyzer::CanCompleteP(size_t q_pos) const {
  return q_pos >= ecg_config::kPWindow;
}


bool ECGAnalyzer::CanCompleteT(size_t s_pos) const {
  return s_pos + ecg_config::kTWindow < samples_.size();
}


size_t ECGAnalyzer::FindLowest(size_t start, size_t end) const {
  if (start >= samples_.size() || end >= samples_.size() || start >= end) {
    return start;
  }
  
  size_t lowest_idx {start};
  float lowest_value {samples_.at(start).voltage};
  
  for (size_t i = start + 1; i <= end; i++) {
    if (samples_.at(i).voltage < lowest_value) {
      lowest_value = samples_.at(i).voltage;
      lowest_idx = i;
    }
  }
  
  return lowest_idx;
}


size_t ECGAnalyzer::FindHighest(size_t start, size_t end) const {
  if (start >= samples_.size() || end >= samples_.size() || start >= end) {
    return start;
  }
  
  size_t highest_idx {start};
  float highest_value {samples_.at(start).voltage};
  
  for (size_t i = start + 1; i <= end; i++) {
    if (samples_.at(i).voltage > highest_value) {
      highest_value = samples_.at(i).voltage;
      highest_idx = i;
    }
  }
  
  return highest_idx;
}


void ECGAnalyzer::TransferProcessedSamples() {
  if (samples_.size() <= ecg_config::kTWindow) {
    return;
  }
  
  size_t safe_transfer_pos {samples_.size() - ecg_config::kTWindow};
  
  if (safe_transfer_pos <= last_transferred_pos_) {
    return;
  }
  
  ApplyClassifications();
  
  // Transfer samples
  for (size_t i = last_transferred_pos_; i < safe_transfer_pos; i++) {
    buffer_classified_->AddData(samples_.at(i));
  }
  
  last_transferred_pos_ = safe_transfer_pos;
  
  // Remove old samples to prevent unbounded growth
  if (last_transferred_pos_ > ecg_config::kTWindow) {
    size_t samples_to_remove {last_transferred_pos_ - ecg_config::kTWindow};
    samples_.erase(samples_.begin(), samples_.begin() + samples_to_remove);
    
    // Adjust all beat indices
    for (auto& beat : detected_beats_) {
      beat.r_pos -= samples_to_remove;
      if (beat.qrs_complete) {
        beat.q_pos -= samples_to_remove;
        beat.s_pos -= samples_to_remove;
      }
      if (beat.p_complete) beat.p_pos -= samples_to_remove;
      if (beat.t_complete) beat.t_pos -= samples_to_remove;
    }
    
    last_transferred_pos_ = ecg_config::kTWindow;
  }
}


void ECGAnalyzer::ApplyClassifications() {
  // Apply wave type classifications to detected beats
  for (const auto& beat : detected_beats_) {
    
    // Always mark R peak
    if (beat.r_pos < samples_.size()) {
      samples_.at(beat.r_pos).classification = WaveType::kR;
    }
    
    // Mark Q and S if QRS is complete
    if (beat.qrs_complete) {
      if (beat.q_pos < samples_.size()) {
        samples_.at(beat.q_pos).classification = WaveType::kQ;
      }
      if (beat.s_pos < samples_.size()) {
        samples_.at(beat.s_pos).classification = WaveType::kS;
      }
    }
    
    // Mark P if detected
    if (beat.p_complete && beat.p_pos < samples_.size()) {
      samples_.at(beat.p_pos).classification = WaveType::kP;
    }
    
    // Mark T if detected
    if (beat.t_complete && beat.t_pos < samples_.size()) {
      samples_.at(beat.t_pos).classification = WaveType::kT;
    }
  }
}


void ECGAnalyzer::ProcessFinalData() {
  LOG_INFO("Processing final data...");
  
  // Process any remaining beats that can be completed
  ProcessCompleteBeats();
  
  // Apply final classifications
  ApplyClassifications();
  
  // Transfer all remaining samples
  for (size_t i = last_transferred_pos_; i < samples_.size(); i++) {
    buffer_classified_->AddData(samples_.at(i));
  }
  
  LOG_INFO("Final processing complete. Total beats detected: %zu",
           detected_beats_.size());
  
  #ifdef DEBUG
  size_t complete_beats {0};
  for (const auto& beat : detected_beats_) {
    if (beat.qrs_complete && beat.p_complete && beat.t_complete) {
      complete_beats++;
    }
  }
  LOG_DEBUG("Complete P-QRS-T beats: %zu/%zu",
            complete_beats, detected_beats_.size());
  #endif
}
