#ifndef ECG_ANALYZER_
#define ECG_ANALYZER_

#include <chrono>

struct Sample {
  float voltage;
  std::chrono::steady_clock::time_point timestamp;
  // TODO (allan): adicionar index?  
};

class ECGAnalyzer {

};

#endif