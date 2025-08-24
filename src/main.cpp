#include "ads1115.h"
#include "logger.h"
#include "ring_buffer.h"
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

int main() {
  std::cout << "==================================" << std::endl;
  std::cout << "Cardiac Monitoring System Starting" << std::endl;
  std::cout << "==================================" << std::endl;

  size_t buffer_size {1280};
  auto ring_buffer {std::make_unique<RingBuffer<float>>(buffer_size)};
  auto ads1115 {std::make_unique<ADS1115>()};

  if(!ads1115->Init()) {
    return 1;
  }

  // Configuração de timing
  constexpr int kSampleRate = 128;  // SPS
  const auto sample_period = std::chrono::microseconds(1000000 / kSampleRate);
  
  LOG_INFO("Starting sampling at %d SPS", kSampleRate);
  LOG_INFO("Sample period: % μs", sample_period.count());

  // Preenche buffer com timing preciso
  auto start_time = std::chrono::high_resolution_clock::now();
  auto next_sample_time = start_time;
  
  size_t samples_collected = 0;
  
  while (!ring_buffer->full()) {
    // Espera até o próximo momento de amostragem
    std::this_thread::sleep_until(next_sample_time);
    
    // Lê o sample
    ring_buffer->add_data(ads1115->ReadVoltage());
    samples_collected++;
    
    // Agenda próxima amostragem
    next_sample_time += sample_period;
    
    // Log de progresso a cada segundo
    /*
    if (samples_collected % kSampleRate == 0) {
      auto current_time = std::chrono::high_resolution_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          current_time - start_time).count();
      
      std::cout << "Collected " << samples_collected << " samples in " 
                << elapsed << "ms" << std::endl;
    }
    */
  }
  
  auto end_time = std::chrono::high_resolution_clock::now();
  auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);
  
  std::cout << "\nSampling complete!" << std::endl;
  std::cout << "Samples collected: " << samples_collected << std::endl;
  std::cout << "Total time: " << total_duration.count() << "ms" << std::endl;
  std::cout << "Expected time: " << (samples_collected * 1000 / kSampleRate)
            << "ms" << std::endl;
  std::cout << "Actual sample rate: " <<
            (samples_collected * 1000.0 / total_duration.count())
            << " SPS" << std::endl;

  // Testa consumo
  std::cout << "\nTesting consumption..." << std::endl;
  for (size_t second = 0; second < 10; second++) {
    std::cout << "Second " << (second + 1) << ": ";
    
    // Consome 128 samples (1 segundo), mas mostra apenas alguns
    for (size_t sample = 0; sample < 128; sample++) {
      auto data = ring_buffer->consume();
      if (sample < 10 && data.has_value()) {
        std::cout << std::fixed << std::setprecision(3) << data.value() << " ";
      }
    }
    std::cout << "..." << std::endl;
  }
  
  return 0;
}