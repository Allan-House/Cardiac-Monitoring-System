#ifndef ECG_ANALYZER_H_
#define ECG_ANALYZER_H_

#include "ring_buffer.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

struct Sample {
  float voltage;
  std::chrono::steady_clock::time_point timestamp;
  // TODO (allan): adicionar index?
  
  Sample() : voltage(0.0f), timestamp(std::chrono::steady_clock::now()) {}

  Sample(float v, std::chrono::time_point<std::chrono::steady_clock> t)
    : voltage(v), timestamp(t) {}
};

class ECGAnalyzer {
  private:
  std::shared_ptr<RingBuffer<Sample>> buffer_raw_;
  std::shared_ptr<RingBuffer<Sample>> buffer_processed_;
  std::atomic<bool> processing_ {false};
  std::thread processing_thread_;

  public:
  ECGAnalyzer(std::shared_ptr<RingBuffer<Sample>> buffer_raw,
              std::shared_ptr<RingBuffer<Sample>> buffer_processed);

  void Run();
  void Stop();

  private:
  void ProcessingLoop();
};

#endif