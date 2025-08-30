#ifndef ECG_ANALYZER_H_
#define ECG_ANALYZER_H_

#include <chrono>

struct Sample {
  float voltage;
  std::chrono::steady_clock::time_point timestamp;
  // TODO (allan): adicionar index?
  
  Sample() : voltage(0.0f), timestamp(std::chrono::steady_clock::now()) {}

  Sample(float v, std::chrono::time_point<std::chrono::steady_clock> t)
    : voltage(v), timestamp(t) {}
};

class ECGAnalyzer {

};

#endif