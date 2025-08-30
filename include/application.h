#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "ads1115.h"
#include "ecg_analyzer.h"
#include "ring_buffer.h"
#include <atomic>
#include <memory>
#include <thread>

class Application {
  private:
  //
  std::shared_ptr<ADS1115> ads1115_;
  std::shared_ptr<RingBuffer<Sample>> buffer_;

  //
  std::atomic<bool> running_ {false};
  std::thread acquisition_thread_;

  // TODO (allan): tentar encontrar uma forma de correlacionar com a configuração
  //               do ADS1115, mesmo que deixe de ser uma constexpr.
  // Timing configurations
  static constexpr uint16_t kSampleRate{250}; // SPS
  static constexpr double kPeriodSeconds{1.0 / kSampleRate}; // 0.004 segundos
  static constexpr auto kPeriodUs{static_cast<int>(kPeriodSeconds * 1000000)}; // 4000 μs
  static constexpr auto kSamplePeriod{std::chrono::microseconds(kPeriodUs)};

  void AcquisitionLoop();

  public:
  Application(std::shared_ptr<ADS1115> ads1115, 
              std::shared_ptr<RingBuffer<Sample>> buffer);
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  ~Application();


  bool Start();
  void Run();
  void Stop();

};

#endif
