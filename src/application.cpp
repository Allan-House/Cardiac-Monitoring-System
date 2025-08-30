#include "ads1115.h"
#include "application.h"
#include "ecg_analyzer.h"
#include "logger.h"
#include "ring_buffer.h"
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

Application::Application(std::shared_ptr<ADS1115> ads1115, 
              std::shared_ptr<RingBuffer<Sample>> buffer)
  : ads1115_ {ads1115}, buffer_ {buffer}
{
  // Empty constructor
}

Application::~Application() {
  Stop();
}

bool Application::Start() {
  LOG_INFO("Sample rate: %d SPS", static_cast<int>(kSampleRate));
  LOG_INFO("Sample period: %d μs", kPeriodUs);
  LOG_INFO("Buffer capacity: %zu samples", buffer_->capacity());
  LOG_INFO("Expected buffer duration: %.1f seconds", 
            static_cast<float>(buffer_->capacity()) / kSampleRate);
  
  if(!ads1115_->Init()) {
    return false;
  }

  return true;
}

// TODO (allan): adicionar callback de controle:
//               - Verificação de sinais (CTRL+C)
//               - Condições de parada específicas
//               - Monitoramento de sistema
void Application::Run() {
  LOG_INFO("Starting acquisition thread...");
  running_ = true;
  acquisition_thread_ = std::thread(&Application::AcquisitionLoop, this);

  std::this_thread::sleep_for(std::chrono::seconds(10));
  Stop();
  
  /*
  if (acquisition_thread_.joinable()) {
    acquisition_thread_.join();
  }
  */
}

void Application::Stop() {
  if (running_) {
    LOG_INFO("Stopping acquisition...");
    running_ = false;
    if (acquisition_thread_.joinable()) {
      acquisition_thread_.join();
    }
    LOG_INFO("Acquisition thread stopped");
  }
}

// TODO (allan): adicionar tratamento de exceção?
void Application::AcquisitionLoop() {
  auto next_sample_time {std::chrono::steady_clock::now()};
  #ifdef DEBUG
    uint32_t sample_count{0};
  #endif

  while (running_) {
    std::this_thread::sleep_until(next_sample_time);

    Sample sample{ads1115_->ReadVoltage(), std::chrono::steady_clock::now()};
    buffer_->add_data(sample);

    next_sample_time += kSamplePeriod;

    #ifdef DEBUG
      sample_count++;
        
      if (sample_count % 250 == 0) {
        std::cout << "Amostras coletadas: " << sample_count 
                  << " | Buffer size: " << buffer_->size() 
                  << " | Última tensão: " << sample.voltage << "V" << std::endl;
      }
    #endif
  }
}