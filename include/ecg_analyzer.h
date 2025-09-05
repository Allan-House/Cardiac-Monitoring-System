#ifndef ECG_ANALYZER_H_
#define ECG_ANALYZER_H_

#include "ring_buffer.h"
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

struct ProcessedSample {
  float voltage;
  std::chrono::steady_clock::time_point timestamp;
  // TODO (allan): adicionar index?
  
  ProcessedSample() : voltage(0.0f), timestamp(std::chrono::steady_clock::now()) {}

  ProcessedSample(float v, std::chrono::time_point<std::chrono::steady_clock> t)
    : voltage(v), timestamp(t) {}
};

class ECGAnalyzer {
  private:
  std::shared_ptr<RingBuffer<Sample>> buffer_raw_;
  std::thread processing_thread_;

  public:
  ECGAnalyzer(std::shared_ptr<RingBuffer<Sample>> buffer_raw);

  void Run();

  private:
  void ProcessingLoop();
};

#endif