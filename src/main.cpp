#include "ads1115.h"
#include "ecg_analyzer.h"
#include "logger.h"
#include "ring_buffer.h"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>

/* TODO (allan): Escolhas a serem feitas:
- Instanciar os objetos estaticamente ou dinamicamente?
- No caso de alocação dinâmica, usar unique_ptr ou shared_ptr?
- Argumentos possíveis.
*/
int main(int argc, char** argv) {
  std::cout << "==================================" << std::endl;
  std::cout << "Cardiac Monitoring System Starting" << std::endl;
  std::cout << "==================================" << std::endl;

  auto ads1115 {std::make_unique<ADS1115>()};
  auto buffer {std::make_unique<RingBuffer<Sample>>(1024)};

  if(!ads1115->Init()) {
    return 1;
  }

  // Configuração de timing
  constexpr uint16_t kSampleRate {128}; // SPS
  const auto sample_period {std::chrono::microseconds(1000000 / kSampleRate)};
  auto start_time {std::chrono::high_resolution_clock::now()};
  auto next_sample_time {start_time};

  while (true) {
    std::this_thread::sleep_until(next_sample_time);
    Sample sample {ads1115->ReadVoltage(), std::chrono::steady_clock::now()};
    buffer->add_data(sample);
    next_sample_time += sample_period;
  }

  return 0;
}