#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "ads1115.h"
#include "ecg_analyzer.h"
#include "file_manager.h"
#include "ring_buffer.h"
#include "system_monitor.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>

class Application {
  private:
  // TODO (allan): tentar encontrar uma forma de correlacionar com a configuração
  //               do ADS1115, mesmo que deixe de ser uma constexpr.
  // Timing configurations
  static constexpr uint16_t kSampleRate{250};  // SPS
  static constexpr double kPeriodSeconds{1.0 / kSampleRate};  // 0.004 seconds
  static constexpr auto kPeriodUs{static_cast<int>(kPeriodSeconds * 1000000)};  // 4000 μs
  static constexpr auto kSamplePeriod{std::chrono::microseconds(kPeriodUs)};
  
  // External dependencies
  std::shared_ptr<ADS1115> ads1115_;
  std::shared_ptr<RingBuffer<Sample>> buffer_raw_;
  std::shared_ptr<RingBuffer<ProcessedSample>> buffer_processed_;
  std::shared_ptr<ECGAnalyzer> ecg_analyzer_;
  std::shared_ptr<FileManager> file_manager_;
  std::shared_ptr<SystemMonitor> system_monitor_;

  // Thread management
  std::thread acquisition_thread_;

  // Synchronization
  std::atomic<bool> running_ {false};
  std::condition_variable data_available_;
  std::mutex data_mutex_;

  // Runtime configuration
  std::chrono::seconds acquisition_duration_ {300};

  public:
  Application(std::shared_ptr<ADS1115> ads1115,
              std::shared_ptr<RingBuffer<Sample>> buffer_raw,
              std::shared_ptr<RingBuffer<ProcessedSample>> buffer_processed,
              std::shared_ptr<ECGAnalyzer> ecg_analyzer,
              std::shared_ptr<FileManager> file_manager,
              std::shared_ptr<SystemMonitor> system_monitor);
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  ~Application();

  // Main control functions
  bool Start();
  void Run();
  void Stop();

  // Status
  bool Running() const {return running_.load();}

  private:
  // Thread functions
  void AcquisitionLoop();
};

#endif
