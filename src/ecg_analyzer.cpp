#include "ecg_analyzer.h"
#include "logger.h"
#include "ring_buffer.h"
#include <cstdint>
#include <thread>
#include <iostream>
#include <cstdlib>

ECGAnalyzer::ECGAnalyzer(std::shared_ptr<RingBuffer<Sample>> buffer_raw,
                         std::shared_ptr<RingBuffer<Sample>> buffer_classified)
  : buffer_raw_ {buffer_raw},
    buffer_classified_ {buffer_classified}
{
  // Empty constructor.
}

void ECGAnalyzer::Run() {
  processing_ = true;
  processing_thread_ = std::thread(&ECGAnalyzer::ProcessingLoop, this);
}

void ECGAnalyzer::Stop() {
  LOG_INFO("Stopping ECG processing thread...");

  processing_ = false;

  if (processing_thread_.joinable()) {
    processing_thread_.join();
  }

  LOG_INFO("ECG processing stopped");
}

void ECGAnalyzer::ProcessingLoop() {
  LOG_INFO("Starting processing thread...");

  #ifdef DEBUG
  uint32_t sample_count {0};
  #endif

  // Real-time processing
  while (processing_) {
    Sample raw {buffer_raw_->Consume()};
    
    Sample processed = raw; // TODO: implementar pré-processamento aqui
    processed_samples_.push_back(processed);

    FSMExecute();
  }

  LOG_INFO("Processing remaining samples in buffer...");
  while (!buffer_raw_->Empty()) {
    Sample raw {buffer_raw_->Consume()};
    
    Sample processed = raw; // TODO: implementar pré-processamento aqui
    processed_samples_.push_back(processed);

    FSMExecute();
  }

  // TODO
  // Processar amostras restantes se estivermos aguardando buffer completo
  if (state_ == States::kWaitingBuffer) {
    LOG_WARN("ECG acquisition ended while waiting for complete buffer");
    // Tentar processar com buffer incompleto ou descartar
  }

  LOG_INFO("Processing thread finished.");
}

void ECGAnalyzer::FSMExecute() {
  switch (state_) {
    case States::kSearchingR:
      SearchRPeak();
      break;
    case States::kWaitingBuffer:
      CheckBufferSize();
      break;
    case States::kDetectingQ:
      DetectQPoint();
      state_ = States::kDetectingS;
      break;
    case States::kDetectingS:
      DetectSPoint();
      state_ = States::kDetectingP;
      break;
    case States::kDetectingP:
      DetectPWave();
      state_ = States::kDetectingT;
      break;
    case States::kDetectingT:
      DetectTWave();
      state_ = States::kFinalizing;
      break;
    case States::kFinalizing:
      FinalizeDetection();
      break;
  }
}

void ECGAnalyzer::SearchRPeak() {
  if (processed_samples_.size() < 3) {
    return;
  }

  // Analysis with 1 sample lag
  size_t analyze_index = processed_samples_.size() - 2;
  
  bool is_peak = processed_samples_.at(analyze_index).voltage > processed_samples_.at(analyze_index - 1).voltage &&
                 processed_samples_.at(analyze_index).voltage > processed_samples_.at(analyze_index + 1).voltage &&
                 processed_samples_.at(analyze_index).voltage > kThreshold;

  if (is_peak) {
    r_index_ = analyze_index;
    wave_points_.at(1) = processed_samples_.at(r_index_); // R peak
    state_ = States::kWaitingBuffer;
  }
}

void ECGAnalyzer::CheckBufferSize() {
  // TODO (allan): criar constantes relacionando com as configurações.
  // Calculations for 250 Hz:
  // - 80ms = 20 samples (Q and S)
  // - 200ms = 50 samples (P wave)
  // - 400ms = 100 samples (T wave)
  
  size_t samples_before_r {r_index_};
  size_t samples_after_r {processed_samples_.size() - r_index_ - 1};
  
  // Check what detection is possible with available data
  bool has_qrs_data {samples_before_r >= kSamples80ms && samples_after_r >= kSamples80ms};
  bool has_p_data {samples_before_r >= (kSamples200ms + kSamples80ms)};
  bool has_t_data {samples_after_r >= (kSamples80ms + kSamples400ms)};
  
  if (has_qrs_data) {   
    state_ = States::kDetectingQ;
  }
}

void ECGAnalyzer::DetectQPoint() {  
  size_t begin {r_index_ - kSamples80ms};
  float lowest {processed_samples_.at(begin).voltage};
  q_index_ = begin;
  
  for (size_t i = begin; i < r_index_; i++) {
    if (processed_samples_.at(i).voltage < lowest) {
      lowest = processed_samples_.at(i).voltage;
      q_index_ = i;
    }
  }
  
  wave_points_.at(0) = processed_samples_.at(q_index_); // Q point
}

void ECGAnalyzer::DetectSPoint() { 
  size_t end {r_index_ + kSamples80ms};
  float lowest {processed_samples_.at(r_index_ + 1).voltage};
  s_index_ = r_index_ + 1;
  
  for (size_t i = r_index_ + 1; i <= end; i++) {
    if (processed_samples_.at(i).voltage < lowest) {
      lowest = processed_samples_.at(i).voltage;
      s_index_ = i;
    }
  }
  
  wave_points_.at(2) = processed_samples_.at(s_index_); // S point
}

void ECGAnalyzer::DetectPWave() {
  if (r_index_ < (kSamples200ms + kSamples80ms)) {
    return;
  }

  size_t begin {q_index_ - kSamples200ms};
  float highest {processed_samples_.at(begin).voltage};
  p_index_ = begin;
  
  for (size_t i = begin; i < q_index_; i++) {
    if (processed_samples_.at(i).voltage > highest) {
      highest = processed_samples_.at(i).voltage;
      p_index_ = i;
    }
  }
  
  wave_points_.at(3) = processed_samples_.at(p_index_); // P wave
}

void ECGAnalyzer::DetectTWave() {
  size_t samples_after_r = processed_samples_.size() - r_index_ - 1;
  if (samples_after_r < (kSamples80ms + kSamples400ms)) {
    return;
  }

  size_t end {s_index_ + kSamples400ms};
  float highest {processed_samples_.at(s_index_ + 1).voltage};
  t_index_ = s_index_ + 1;
  
  for (size_t i = s_index_ + 1; i <= end; i++) {
    if (processed_samples_.at(i).voltage > highest) {
      highest = processed_samples_.at(i).voltage;
      t_index_ = i;
    }
  }
  
  wave_points_.at(4) = processed_samples_.at(t_index_); // T wave
}

void ECGAnalyzer::FinalizeDetection() {
  TransferToClassifiedBuffer();
  processed_samples_.clear();
  p_index_ = q_index_ = r_index_ = s_index_ = t_index_ = 0;
  state_ = States::kSearchingR;
  LOG_DEBUG("P-QRS-T detection cycle completed");
}

void ECGAnalyzer::TransferToClassifiedBuffer() {
  for (size_t i = 0; i < processed_samples_.size(); i++) {
    Sample classified_sample = processed_samples_.at(i);
    
    if (i == p_index_) {
      classified_sample.classification = WaveType::kP;
    } else if (i == q_index_) {
      classified_sample.classification = WaveType::kQ;
    } else if (i == r_index_) {
      classified_sample.classification = WaveType::kR;
    } else if (i == s_index_) {
      classified_sample.classification = WaveType::kS;
    } else if (i == t_index_) {
      classified_sample.classification = WaveType::kT;
    }
    
    buffer_classified_->AddData(classified_sample);
  }
  
  LOG_DEBUG("Transferred %zu classified samples to buffer", processed_samples_.size());
}
