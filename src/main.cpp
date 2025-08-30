#include "ads1115.h"
#include "application.h"
#include "ecg_analyzer.h"
#include "logger.h"
#include "ring_buffer.h"
#include <chrono>
#include <iostream>
#include <memory>

/**
 * 250 SPS x 300s -> 75000 samples
  * 475 SPS x 300s -> 142500 samples
*/

/* TODO (allan): Escolhas a serem feitas:
- Instanciar os objetos estaticamente ou dinamicamente?
- Argumentos possíveis.
*/
int main(int argc, char** argv) {
  cardiac_logger::init("cardiac_system.log", cardiac_logger::Level::kTrace);

  std::cout << "==================================" << std::endl;
  std::cout << "Cardiac Monitoring System Starting" << std::endl;
  std::cout << "==================================" << std::endl;

  auto ads1115 {std::make_shared<ADS1115>()};
  auto buffer {std::make_shared<RingBuffer<Sample>>(75000)};

  Application application{ads1115, buffer};

  // TODO (allan): verificação de erro de Application::Start();
  if (application.Start()) {
    application.Run();
  }

  std::cout << "Goodbye!!" << std::endl;
  return 0;
}