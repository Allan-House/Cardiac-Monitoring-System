#include "ecg_analyzer.h"
#include "logger.h"
#include "ring_buffer.h"
#include <cstdint>
#include <thread>
#include <iostream>

ECGAnalyzer::ECGAnalyzer(std::shared_ptr<RingBuffer<Sample>> buffer_raw,
                         std::shared_ptr<RingBuffer<Sample>> buffer_processed)
  : buffer_raw_ {buffer_raw},
    buffer_processed_ {buffer_processed}
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
    Sample raw;
    raw = buffer_raw_->Consume();
    
    Sample processed;
    processed = raw;
    buffer_processed_->AddData(processed);
  }

  LOG_INFO("Processing remaining samples in buffer...");
  while (!buffer_raw_->Empty()) {
    Sample raw;
    raw = buffer_raw_->Consume();
    
    Sample processed;
    processed = raw;
    buffer_processed_->AddData(processed);
  }

  LOG_INFO("Processing thread finished.");
}