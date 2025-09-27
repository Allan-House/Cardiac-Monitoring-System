#ifndef ECG_ANALYZER_H_
#define ECG_ANALYZER_H_

#include "ring_buffer.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

enum class WaveType {
  kNormal,    // Amostra normal (sem classificação específica)
  kP,         // Onda P
  kQ,         // Ponto Q
  kR,         // Pico R
  kS,         // Ponto S
  kT          // Onda T
};

struct Sample {
  float voltage;
  std::chrono::steady_clock::time_point timestamp;
  WaveType classification {WaveType::kNormal};
  
  Sample() : voltage(0.0f), timestamp(std::chrono::steady_clock::now()) {}

  Sample(float v, std::chrono::time_point<std::chrono::steady_clock> t)
    : voltage(v), timestamp(t) {}

  Sample(float v, std::chrono::time_point<std::chrono::steady_clock> t, WaveType type)
    : voltage(v), timestamp(t), classification(type) {}
};

// Configuration constants (centralized)
namespace ecg_config {
  constexpr uint16_t kSampleRate = 250;  // SPS
  constexpr float kRThreshold = 2.0f;    // R peak threshold
  
  // Detection windows (in samples) - exactly from the paper
  constexpr size_t kQSWindow = kSampleRate * 80 / 1000;   // 80ms = 20 samples
  constexpr size_t kPWindow = kSampleRate * 200 / 1000;   // 200ms = 50 samples  
  constexpr size_t kTWindow = kSampleRate * 400 / 1000;   // 400ms = 100 samples
  
  // Refractory period to avoid double detection
  constexpr size_t kRefractoryPeriod = kSampleRate * 300 / 1000; // 300ms = 75 samples
}

struct Beat {
  size_t r_pos;
  size_t q_pos = 0;
  size_t s_pos = 0;
  size_t p_pos = 0;
  size_t t_pos = 0;
  bool qrs_complete = false;
  bool p_complete = false;
  bool t_complete = false;
  
  Beat(size_t r) : r_pos(r) {}
};

class ECGAnalyzer {
  private:
  // Buffers
  std::vector<Sample> sample_buffer_;
  std::vector<Beat> detected_beats_;
  std::shared_ptr<RingBuffer<Sample>> buffer_raw_;
  std::shared_ptr<RingBuffer<Sample>> buffer_classified_;
  
  // Thread control
  std::atomic<bool> processing_ {false};
  std::thread processing_thread_;
  
  // State tracking
  size_t last_transferred_pos_ = 0;

  public:
  ECGAnalyzer(std::shared_ptr<RingBuffer<Sample>> buffer_raw,
              std::shared_ptr<RingBuffer<Sample>> buffer_classified);

  void Run();
  void Stop();

  private:
  void ProcessingLoop();
  void ProcessSample(const Sample& sample);
  
  void DetectRPeaks();
  void ProcessCompleteBeats();
  void TransferProcessedSamples();
  void ProcessFinalData();
  
  bool IsRPeak(size_t pos) const;
  bool CanCompleteQRS(size_t r_pos) const;
  bool CanCompleteP(size_t q_pos) const;
  bool CanCompleteT(size_t s_pos) const;
  
  size_t FindLowest(size_t start, size_t end) const;
  size_t FindHighest(size_t start, size_t end) const;
  
  void ApplyClassifications();
};

#endif
