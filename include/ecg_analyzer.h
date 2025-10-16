#ifndef ECG_ANALYZER_H_
#define ECG_ANALYZER_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "config.h"
#include "ring_buffer.h"

/**
 * @brief ECG-specific configuration parameters.
 */
namespace ecg_config {
  constexpr float kRThreshold {2.5f};
  
  // Detection windows (in samples)
  constexpr size_t kQSWindow {config::kSampleRate * 80 / 1000}; // 80ms
  constexpr size_t kPWindow {config::kSampleRate * 200 / 1000}; // 200ms
  constexpr size_t kTWindow {config::kSampleRate * 400 / 1000}; // 400ms
  
  // Refractory period to avoid double detection
  constexpr size_t kRefractoryPeriod {config::kSampleRate * 300 / 1000}; // 300ms
}

/**
 * @brief Classification types for ECG wave components.
 */
enum class WaveType {
  kNormal,
  kP,
  kQ,
  kR,
  kS,
  kT
};

/**
 * @brief ECG sample with voltage, timestamp and wave classification.
 */
struct Sample {
  float voltage;
  std::chrono::steady_clock::time_point timestamp;
  WaveType classification {WaveType::kNormal};
  
  /**
   * @brief Default constructor - initializes with zero voltage and current time.
   */
  Sample() : voltage(0.0f), timestamp(std::chrono::steady_clock::now()) {}

  /**
   * @brief Constructs sample with voltage and timestamp.
   * 
   * @param v Voltage value in volts
   * @param t Timestamp of acquisition
   */
  Sample(float v, std::chrono::time_point<std::chrono::steady_clock> t)
    : voltage(v), timestamp(t) {}

  /**
   * @brief Constructs sample with full classification.
   * 
   * @param v Voltage value in volts
   * @param t Timestamp of acquisition
   * @param type Wave type classification
   */
  Sample(float v, std::chrono::time_point<std::chrono::steady_clock> t, WaveType type)
    : voltage(v), timestamp(t), classification(type) {}
};

/**
 * @brief Represents a detected heartbeat with PQRST complex positions.
 */
struct Beat {
  size_t r_pos;
  size_t q_pos = 0;
  size_t s_pos = 0;
  size_t p_pos = 0;
  size_t t_pos = 0;
  bool qrs_complete = false;
  bool p_complete = false;
  bool t_complete = false;
  
  /**
   * @brief Constructs beat with R peak position.
   * 
   * @param r R peak position in samples buffer
   */
  Beat(size_t r) : r_pos(r) {}
};

/**
 * @brief Real-time ECG signal analyzer for PQRST complex detection.
 * 
 * Processes raw ECG samples to identify and classify cardiac wave
 * components (P, Q, R, S, T waves) using threshold-based peak detection
 * and temporal windowing. Runs in a separate thread consuming samples
 * from the raw buffer and producing classified samples.
 * 
 * Algorithm overview:
 * 1. Detects R peaks using voltage threshold and refractory period
 * 2. Identifies Q and S waves as minima around R peak (±80ms)
 * 3. Locates P wave as maximum before QRS complex (200ms window)
 * 4. Finds T wave as maximum after QRS complex (400ms window)
 */
class ECGAnalyzer {
  private:
  // Buffers
  std::vector<Sample> samples_;
  std::vector<Beat> detected_beats_;
  std::shared_ptr<RingBuffer<Sample>> buffer_raw_;
  std::shared_ptr<RingBuffer<Sample>> buffer_classified_;
  
  // Thread control
  std::atomic<bool> processing_ {false};
  std::thread processing_thread_;
  
  // State tracking
  size_t last_transferred_pos_ {0};

  public:
  /**
   * @brief Constructs ECG analyzer with input/output buffers.
   * 
   * @param buffer_raw Input buffer containing raw ECG samples
   * @param buffer_classified Output buffer for classified samples
   */
  ECGAnalyzer(std::shared_ptr<RingBuffer<Sample>> buffer_raw,
              std::shared_ptr<RingBuffer<Sample>> buffer_classified);

  /**
   * @brief Starts the processing thread.
   * 
   * Begins consuming samples from raw buffer and producing
   * classified samples in the output buffer.
   */
  void Run();

  /**
   * @brief Stops processing and waits for thread completion.
   * 
   * Processes remaining samples before shutting down.
   */
  void Stop();

  private:
  /**
   * @brief Main processing loop - consumes and processes samples.
   * 
   * Runs until stopped, processing each sample through the
   * detection pipeline and transferring classified results.
   */
  void ProcessingLoop();

  /**
   * @brief Processes single sample through detection pipeline.
   * 
   * @param sample Input sample to process
   */
  void ProcessSample(const Sample& sample);
  
  /**
   * @brief Detects R peaks in recent samples.
   * 
   * Uses threshold detection with refractory period to
   * identify QRS complex R peaks.
   */
  void DetectRPeaks();

  /**
   * @brief Completes PQRST detection for identified beats.
   * 
   * Processes detected R peaks to find associated P, Q, S, T waves
   * when sufficient surrounding samples are available.
   */
  void ProcessCompleteBeats();

  /**
   * @brief Transfers processed samples to output buffer.
   * 
   * Moves samples with classifications to the output buffer,
   * maintaining a window for ongoing detection.
   */
  void TransferProcessedSamples();

  /**
   * @brief Final processing during shutdown.
   * 
   * Completes any pending beat classifications and transfers
   * all remaining samples.
   */
  void ProcessFinalData();
  
  /**
   * @brief Checks if position is an R peak.
   * 
   * @param pos Position in samples buffer to check
   * @return true if position is R peak, false otherwise
   */
  bool IsRPeak(size_t pos) const;

  /**
   * @brief Checks if QRS complex can be completed.
   * 
   * @param r_pos R peak position
   * @return true if enough samples exist for Q and S detection
   */
  bool CanCompleteQRS(size_t r_pos) const;

  /**
   * @brief Checks if P wave can be detected.
   * 
   * @param q_pos Q wave position
   * @return true if enough prior samples exist for P detection
   */
  bool CanCompleteP(size_t q_pos) const;

  /**
   * @brief Checks if T wave can be detected.
   * 
   * @param s_pos S wave position
   * @return true if enough following samples exist for T detection
   */
  bool CanCompleteT(size_t s_pos) const;
  
  /**
   * @brief Finds lowest voltage point in range.
   * 
   * @param start Start position (inclusive)
   * @param end End position (inclusive)
   * @return Position of minimum voltage in range
   */
  size_t FindLowest(size_t start, size_t end) const;
  
  /**
   * @brief Finds highest voltage point in range.
   * 
   * @param start Start position (inclusive)
   * @param end End position (inclusive)
   * @return Position of maximum voltage in range
   */
  size_t FindHighest(size_t start, size_t end) const;
  
  /**
   * @brief Applies wave classifications to samples.
   * 
   * Updates classification field of samples based on
   * detected beat positions.
   */
  void ApplyClassifications();
};

#endif
