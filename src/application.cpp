#include "application.h"

#include <cstdint>

#include "logger.h"

Application::Application(std::shared_ptr<DataSource> data_source,
                         std::shared_ptr<RingBuffer<Sample>> buffer_raw,
                         std::shared_ptr<RingBuffer<Sample>> buffer_classified,
                         std::unique_ptr<ECGAnalyzer> ecg_analyzer,
                         std::unique_ptr<FileManager> file_manager,
                         std::unique_ptr<SystemMonitor> system_monitor,
                         std::unique_ptr<TCPFileServer> tcp_server)
  : data_source_ {data_source},
    buffer_raw_ {buffer_raw},
    buffer_classified_ {buffer_classified},
    ecg_analyzer_ {std::move(ecg_analyzer)},
    file_manager_ {std::move(file_manager)},
    system_monitor_ {std::move(system_monitor)},
    tcp_server_ {std::move(tcp_server)}
{
  // Empty constructor
}


Application::~Application() {
  Stop();
}


bool Application::Start() {
  LOG_INFO("Starting application...");
  LOG_INFO("Sample rate: %d SPS", static_cast<int>(config::kSampleRate));
  LOG_INFO("Sample period: %d μs", config::kPeriodUs);
  
  if (!data_source_->Available()) {
    LOG_ERROR("Data source not available");
    return false;
  }

  if (!file_manager_->Init()) {
    return false;
  }

  #ifdef USE_HARDWARE_SOURCE
  if (tcp_server_ && !tcp_server_->Init()) {
    LOG_ERROR("Failed to initialize TCP server");
    return false;
  }
  #endif

  LOG_SUCCESS("All components initialized successfully");
  running_ = true;
  return true;
}


// TODO (allan): adicionar callback de controle:
//               - Verificação de sinais (CTRL+C)
//               - Condições de parada específicas
//               - Monitoramento de sistema
void Application::Run() {
  if (!running_.load()) {
    LOG_ERROR("Application not started. Call Start() first.");
    return;
  }

  // Start all processing threads
  acquisition_thread_ = std::thread(&Application::AcquisitionLoop, this);
  ecg_analyzer_->Run();
  file_manager_->Run();

  #ifdef USE_HARDWARE_SOURCE
  if (tcp_server_) {
    tcp_server_->Run();
  }
  #endif

  LOG_INFO("Waiting for acquisition to complete...");

  // Wait for either acquisition completion or shutdown signal
  while (running_.load() && !shutdown_requested_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  
  // Check why we exited the loop
  if (shutdown_requested_.load()) {
    LOG_WARN("Shutdown requested by signal, initiating graceful shutdown...");
    GracefulShutdown();
  } else {
    LOG_INFO("Acquisition finished normally, stopping processing threads...");
    
    if (acquisition_thread_.joinable()) {
      acquisition_thread_.join();
    }

    ecg_analyzer_->Stop();
    file_manager_->Stop();
    
    #ifdef USE_HARDWARE_SOURCE
    if (tcp_server_) {
      LOG_INFO("Sending files to connected client (if any)...");
      tcp_server_->SendAvailableFiles();
      tcp_server_->Stop();
    }
    #endif
  }

  LOG_SUCCESS("All threads stopped successfully");
}


void Application::Stop() {
  if (running_) {
    LOG_INFO("Stopping application...");
    running_ = false;

    buffer_raw_->Shutdown();
    
    LOG_SUCCESS("Application stopped");
  }
}


void Application::AcquisitionLoop() {
  LOG_INFO("Starting acquisition for %ld seconds.", acquisition_duration_.count());

  auto start_time {std::chrono::steady_clock::now()};
  auto end_time {start_time + acquisition_duration_};
  
  #ifdef DEBUG
  uint32_t sample_count{0};
  #endif
  
  uint32_t expected_sample {0};
  
  try {
    while (running_.load() && 
           !shutdown_requested_.load() && 
           std::chrono::steady_clock::now() < end_time) {
      expected_sample++;
      
      auto target_time {start_time + (expected_sample * config::kSamplePeriod)};
      std::this_thread::sleep_until(target_time);
      
      // Check if we should stop (either flag)
      if (!running_.load() || shutdown_requested_.load()) {
        break;
      }
      
      auto voltage {data_source_->ReadVoltage()};

      if (!voltage) {
        LOG_WARN("Failed to read voltage, skipping sample");
        continue;
      }

      Sample sample{voltage.value(), std::chrono::steady_clock::now()};
      buffer_raw_->AddData(sample);

      #ifdef DEBUG
      sample_count++;
      if (sample_count % config::kSampleRate == 0) {
        std::cout << "Samples collected: " << sample_count 
                  << std::endl;
      }
      #endif
      
      // Auto-correction: if too late, skip a sample
      auto now {std::chrono::steady_clock::now()};
      auto delay {std::chrono::duration_cast<std::chrono::microseconds>(now - target_time).count()};
      
      if (delay > 2000) { // If more than 2ms late
        LOG_WARN("High delay detected: %ldμs, skipping to catch up", delay);
        expected_sample = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count() / 4000;
      }
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Exception in acquisition loop: %s", e.what());
  }

  if (shutdown_requested_.load()) {
    LOG_INFO("Acquisition interrupted by shutdown signal");
  } else {
    LOG_INFO("Acquisition duration completed");
  }
  
  // Signal that acquisition is done
  running_ = false;
  
  // Signal shutdown to raw buffer to wake up waiting threads
  buffer_raw_->Shutdown();
  
  LOG_INFO("Acquisition loop finished");
}


void Application::RequestShutdown() {
  LOG_WARN("Shutdown requested via signal handler");
  shutdown_requested_.store(true);
  running_.store(false);
}


void Application::GracefulShutdown() {
  LOG_INFO("Starting graceful shutdown sequence...");
  
  // Step 1: Stop acquisition immediately
  running_.store(false);
  
  if (acquisition_thread_.joinable()) {
    LOG_INFO("Waiting for acquisition thread to finish...");
    acquisition_thread_.join();
    LOG_SUCCESS("Acquisition thread stopped");
  }
  
  // Step 2: Stop processing threads
  LOG_INFO("Stopping ECG analyzer...");
  ecg_analyzer_->Stop();
  LOG_SUCCESS("ECG analyzer stopped");
  
  // Step 3: Flush and close files
  LOG_INFO("Flushing file buffers...");
  file_manager_->Stop();
  LOG_SUCCESS("File manager stopped, all data saved");
  
  // Step 4: Handle TCP connections
  #ifdef USE_HARDWARE_SOURCE
  if (tcp_server_) {
    if (tcp_server_->HasConnectedClient()) {
      LOG_INFO("Client connected, attempting to send files...");
      tcp_server_->SendAvailableFiles();
    }
    LOG_INFO("Stopping TCP server...");
    tcp_server_->Stop();
    LOG_SUCCESS("TCP server stopped");
  }
  #endif
  
  LOG_SUCCESS("Graceful shutdown completed");
}


void Application::set_acquisition_duration(std::chrono::seconds duration) {
  acquisition_duration_ = duration;
}
