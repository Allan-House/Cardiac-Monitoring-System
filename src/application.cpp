#include "ads1115.h"
#include "application.h"
#include "ecg_analyzer.h"
#include "file_manager.h"
#include "logger.h"
#include "ring_buffer.h"
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

Application::Application(std::shared_ptr<ADS1115> ads1115, 
                         std::shared_ptr<RingBuffer<Sample>> buffer,
                         std::shared_ptr<FileManager> file_manager)
  : ads1115_ {ads1115}, buffer_ {buffer}, file_manager_ {file_manager}
{
  // Empty constructor
}

Application::~Application() {
  Stop();
}

bool Application::Start() {
  LOG_INFO("Starting application...");
  LOG_INFO("Sample rate: %d SPS", static_cast<int>(kSampleRate));
  LOG_INFO("Sample period: %d μs", kPeriodUs);
  
  if(!ads1115_->Init()) {
    return false;
  }

  if (!file_manager_->Init()) {
    LOG_ERROR("Failed to initialize FileManager");
    return false;
  }

  LOG_SUCCESS("All components initialized successfully");
  return true;
}

// TODO (allan): adicionar callback de controle:
//               - Verificação de sinais (CTRL+C)
//               - Condições de parada específicas
//               - Monitoramento de sistema
void Application::Run() {
  running_ = true;
  
  // TODO (allan): refatorar (provavelmente)
  acquisition_thread_ = std::thread(&Application::AcquisitionLoop, this);
  writing_thread_ = std::thread([this]() { 
      file_manager_->WriterLoop(running_); 
  });
  
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
    running_ = false;

    if (acquisition_thread_.joinable()) {
      acquisition_thread_.join();
    }
        
    if (writing_thread_.joinable()) {
      writing_thread_.join();
    }

    file_manager_->Close();

  }

  LOG_SUCCESS("Application stopped");
}

// TODO (allan): adicionar tratamento de exceção?
void Application::AcquisitionLoop() {
  LOG_INFO("Starting acquisition thread...");
  auto start_time = std::chrono::steady_clock::now();
  uint32_t expected_sample {0};
  #ifdef DEBUG
    uint32_t sample_count{0};
  #endif
  
  while (running_) {
    expected_sample++;
    
    auto target_time = start_time + (expected_sample * kSamplePeriod);
    
    std::this_thread::sleep_until(target_time);
    
    Sample sample{ads1115_->ReadVoltage(), std::chrono::steady_clock::now()};
    buffer_->AddData(sample);

    #ifdef DEBUG
      sample_count++;
      
      // TODO (allan): tentar usar constante em vez de número mágico.
      if (sample_count % 250 == 0) {
        std::cout << "Samples collected: " << sample_count 
                  << " | Buffer size: " << buffer_->Size()
                  << " | Last voltage: " << sample.voltage << "V" << std::endl;
      }
    #endif
    
    // Auto-correction: if too late, skip a sample
    auto now = std::chrono::steady_clock::now();
    auto delay = std::chrono::duration_cast<std::chrono::microseconds>(now - target_time).count();
    
    if (delay > 2000) { // If more than 2ms late
      LOG_WARN("High delay detected: %ldμs, skipping to catch up", delay);
      expected_sample = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count() / 4000;
    }
  }

  LOG_INFO("Stopping acquisition thread...");
}

void Application::WritingLoop() {
  file_manager_->WriterLoop(running_);
}