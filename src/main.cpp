#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <cstring>

#include "application.h"
#include "config.h"
#include "data_source.h"
#include "ecg_analyzer.h"
#include "file_manager.h"
#include "logger.h"
#include "ring_buffer.h"
#include "system_monitor.h"
#include "tcp_file_server.h"
#include "signal_handler.h"

#ifdef USE_HARDWARE_SOURCE
#include "ads1115.h"
#include "sensor_data.h"
#endif

#ifdef USE_FILE_SOURCE
#include "file_data.h"
#endif

static Application* g_application = nullptr;

std::shared_ptr<DataSource> createDataSource(int argc, char** argv);
void print_help(const char* program_name);
bool parse_arguments(int argc, char** argv,
                    std::string& data_file, 
                    std::chrono::seconds& duration);

int main(int argc, char** argv) {
  std::string data_file;
  std::chrono::seconds acquisition_duration {config::kAcquisitionDuration};

  if (!parse_arguments(argc, argv, data_file, acquisition_duration)) {
    return 1;
  }

  std::cout << "==================================" << std::endl;
  std::cout << "Cardiac Monitoring System Starting" << std::endl;
  std::cout << "==================================" << std::endl;
  
  logger::Level log_level;
  #ifdef DEBUG
  log_level = logger::Level::kDebug;
  #else
  log_level = logger::Level::kInfo;
  #endif
  logger::init(config::kDefaultLogFile, log_level);
  
  // Create data source based on compilation target
  auto data_source = createDataSource(argc, argv);
  
  if (!data_source || !data_source->Available()) {
    std::cerr << "Failed to initialize data source" << std::endl;
    return 1;
  }

  auto buffer_raw {std::make_shared<RingBuffer<Sample>>(config::kBufferSize)};
  auto buffer_classified {std::make_shared<RingBuffer<Sample>>(config::kBufferSize)};
  auto ecg_analyzer {std::make_unique<ECGAnalyzer>(buffer_raw, buffer_classified)};
  auto file_manager {std::make_unique<FileManager>(
    buffer_classified,
    "cardiac_data",
    config::kFileWriteInterval
  )};
  auto system_monitor {std::make_unique<SystemMonitor>()};
  std::unique_ptr<TCPFileServer> tcp_server;

  #ifdef USE_HARDWARE_SOURCE
  tcp_server = std::make_unique<TCPFileServer>();
  #endif

  Application application{
    data_source,
    buffer_raw,
    buffer_classified,
    std::move(ecg_analyzer),
    std::move(file_manager),
    std::move(system_monitor),
    std::move(tcp_server)
  };

  application.set_acquisition_duration(acquisition_duration);

  // Install signal handlers BEFORE starting application
  g_application = &application;
  
  if (!SignalHandler::Init([&application]() {
    // This callback runs in signal handler context
    application.RequestShutdown();
  })) {
    LOG_ERROR("Failed to install signal handlers");
    return 1;
  }
  
  LOG_INFO("Signal handlers installed (Ctrl+C or SIGTERM to stop)");

  if (!application.Start()) {
    LOG_ERROR("Failed to start application");
    return 1;
  }
   
  application.Run();

  LOG_SUCCESS("Application completed successfully");

  logger::shutdown();
  return 0;
}

void print_help(const char* program_name) {
  std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
  std::cout << "Cardiac Monitoring System - ECG acquisition and analysis\n\n";
  
  std::cout << "OPTIONS:\n";
  
  #ifdef USE_FILE_SOURCE
    std::cout << "  <filename>              Input ECG data file";
    std::cout << " (default: data/ecg_samples.bin)\n";
  #endif
  
  std::cout << "  -d, --duration <sec>    Acquisition duration in seconds";
  std::cout << " (default: " << config::kAcquisitionDuration.count() << ")\n";
  std::cout << "  -h, --help              Show this help message\n\n";
  
  std::cout << std::endl;
}

bool parse_arguments(int argc, char** argv,
                     std::string& data_file,
                     std::chrono::seconds& duration) {
  duration = config::kAcquisitionDuration;
  
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    
    // Help
    if (arg == "-h" || arg == "--help") {
      print_help(argv[0]);
      exit(0);
    }
    
    // Duration
    if (arg == "-d" || arg == "--duration") {
      if (i + 1 >= argc) {
        std::cerr << "Error: --duration requires an argument\n";
        print_help(argv[0]);
        return false;
      }
      
      int dur {std::stoi(argv[++i])};
      duration = std::chrono::seconds(dur);
      continue;
    }
    
    #ifdef USE_FILE_SOURCE
    // File argument (only for file source builds)
    if (arg[0] != '-') {
      data_file = arg;
      continue;
    }
    #endif
    
    // Unknown argument
    std::cerr << "Error: unknown argument '" << arg << "'\n";
    print_help(argv[0]);
    return false;
  }
  
  return true;
}

std::shared_ptr<DataSource> createDataSource(int argc, char** argv) {
#ifdef USE_HARDWARE_SOURCE
  auto ads1115 = std::make_shared<ADS1115>();
  return std::make_shared<SensorData>(ads1115);
  
#elif defined(USE_FILE_SOURCE)
  std::string filename = "data/ecg_samples.bin"; // Default file
  
  // Find filename in arguments (skip options)
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg[0] != '-' && arg != "--duration" && 
        (i == 0 || (argv[i-1] != std::string("-d") && argv[i-1] != std::string("--duration")))) {
      filename = arg;
      break;
    }
  }
  
  return std::make_shared<FileData>(filename);
  
#else
  #error "No data source defined. Check CMake configuration."
#endif
}
