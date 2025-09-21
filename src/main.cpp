#include <iostream>
#include <memory>
#include <string>
#include <chrono>

#include "data_source.h"
#include "logger.h"
#include "ring_buffer.h"
#include "ecg_analyzer.h"
#include "file_manager.h"
#include "system_monitor.h"
#include "application.h"

#ifdef USE_HARDWARE_SOURCE
  #include "ads1115.h"
  #include "sensor_data.h"
#endif

#ifdef USE_FILE_SOURCE
  #include "file_data.h"
#endif

std::shared_ptr<DataSource> createDataSource(int argc, char** argv);

/**
 * 250 SPS x 300s -> 75000 samples
 * 475 SPS x 300s -> 142500 samples
*/

/* TODO (allan): Escolhas a serem feitas:
- Instanciar os objetos estaticamente ou dinamicamente?
- Argumentos possíveis.
*/
int main(int argc, char** argv) {
  cardiac_logger::init("cardiac_system.log", cardiac_logger::Level::kDebug);

  std::cout << "==================================" << std::endl;
  std::cout << "Cardiac Monitoring System Starting" << std::endl;
  std::cout << "==================================" << std::endl;

  // Create data source based on compilation target
  auto data_source = createDataSource(argc, argv);
  
  if (!data_source || !data_source->Available()) {
    std::cerr << "Failed to initialize data source" << std::endl;
    return 1;
  }

  auto buffer_raw = std::make_shared<RingBuffer<Sample>>(75000);
  auto buffer_classified = std::make_shared<RingBuffer<Sample>>(75000);
  auto ecg_analyzer = std::make_shared<ECGAnalyzer>(buffer_raw, buffer_classified);
  auto file_manager = std::make_shared<FileManager>(
    buffer_classified,
    "cardiac_data.bin",
    "cardiac_data.csv",
    std::chrono::milliseconds(200) // 50 samples per write
  );
  auto system_monitor = std::make_shared<SystemMonitor>();
  
  Application application{data_source,
                          buffer_raw,
                          buffer_classified,
                          ecg_analyzer,
                          file_manager,
                          system_monitor};

  // TODO (allan): verificação de erro de Application::Start();
  if (application.Start()) {
    application.Run();
  }

  return 0;
}

std::shared_ptr<DataSource> createDataSource(int argc, char** argv) {
#ifdef USE_HARDWARE_SOURCE
  std::cout << "Using hardware data source (Raspberry Pi)" << std::endl;
  auto ads1115 = std::make_shared<ADS1115>();
  return std::make_shared<SensorData>(ads1115);
  
#elif defined(USE_FILE_SOURCE)
  std::string filename = "/workspaces/Cardiac-Monitoring-System/data/ecg_samples.bin"; // Default file
  
  if (argc > 1) {
    filename = argv[1];
    std::cout << "Using file data source: " << filename << std::endl;
  } else {
    std::cout << "Using default file data source: " << filename << std::endl;
  }
  
  return std::make_shared<FileData>(filename);
  
#else
  #error "No data source defined. Check CMake configuration."
#endif
}
