#include "ecg_analyzer.h"
#include "logger.h"
#include "ring_buffer.h"

#include <thread>

ECGAnalyzer::ECGAnalyzer(std::shared_ptr<RingBuffer<Sample>> buffer_raw)
  : buffer_raw_ {buffer_raw}
{
  // Empty constructor.
}

void ECGAnalyzer::Run() {
  processing_thread_ = std::thread(ECGAnalyzer::ProcessingLoop, this);
}

void ECGAnalyzer::ProcessingLoop() {
  LOG_INFO("Starting processing thread...");

  Sample sample;
  
  // TODO (allan): definir tempo de sleep 
  while (!buffer_raw_->Empty()) {
    sample = buffer_raw_->Consume();
    // processa a sample
    // adiciona a sample processada a um buffer de dados processados.
  }

  LOG_INFO("Stopping processing thread...");
}