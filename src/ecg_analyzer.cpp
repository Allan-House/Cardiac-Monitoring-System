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
  LOG_INFO("Starting ECG processing with filtering enabled");
  
  #ifdef DEBUG
  uint32_t sample_count {0};
  #endif

  // Prime the filter with the first sample
  if (!buffer_raw_->Empty()) {
    Sample first = buffer_raw_->Consume();
    filter_.Initialize(first.voltage);

    Sample processed;
    processed.voltage = filter_.Process(first.voltage);
    processed.timestamp = first.timestamp;
    buffer_processed_->AddData(processed);

    #ifdef DEBUG
    sample_count++;
    #endif
  }

  // Real-time processing
  while (processing_) {
    Sample raw = buffer_raw_->Consume();
    
    // Apply filtering
    Sample processed;
    processed.voltage = filter_.Process(raw.voltage);
    processed.timestamp = raw.timestamp;
    
    buffer_processed_->AddData(processed);
    
    #ifdef DEBUG
    sample_count++;
    if (sample_count % 1000 == 0) {
      LOG_DEBUG("Processed %u samples | Raw: %.4fV | Filtered: %.4fV",
                sample_count, raw.voltage, processed.voltage);
    }
    #endif
  }

  LOG_INFO("Processing remaining samples in buffer...");
  while (!buffer_raw_->Empty()) {
    Sample raw = buffer_raw_->Consume();
    
    Sample processed;
    processed.voltage = filter_.Process(raw.voltage);
    processed.timestamp = raw.timestamp;
    buffer_processed_->AddData(processed);
    
    #ifdef DEBUG
    sample_count++;
    #endif
  }

  #ifdef DEBUG
  LOG_INFO("ECG filtering completed for %u samples", sample_count);
  #endif
  LOG_INFO("Processing thread finished.");
}
