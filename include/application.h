#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "config.h"
#include "data_source.h"
#include "ecg_analyzer.h"
#include "file_manager.h"
#include "ring_buffer.h"
#include "system_monitor.h"
#include "tcp_file_server.h"

class Application {
  private:
  
  // External dependencies
  std::shared_ptr<DataSource> data_source_;
  std::shared_ptr<RingBuffer<Sample>> buffer_raw_;
  std::shared_ptr<RingBuffer<Sample>> buffer_classified_;
  std::unique_ptr<ECGAnalyzer> ecg_analyzer_;
  std::unique_ptr<FileManager> file_manager_;
  std::unique_ptr<SystemMonitor> system_monitor_;
  std::unique_ptr<TCPFileServer> tcp_server_;

  // Thread management
  std::thread acquisition_thread_;
  
  // Synchronization
  std::atomic<bool> running_ {false};
  std::atomic<bool> shutdown_requested_ {false};
  
  // Runtime configuration
  std::chrono::seconds acquisition_duration_ {config::kAcquisitionDuration};

  public:

  /**
   * @brief Constructs application with all required components.
   * 
   * @param data_source Data source for ECG samples
   * @param buffer_raw Buffer for raw acquired samples
   * @param buffer_classified Buffer for analyzed samples
   * @param ecg_analyzer ECG analysis component
   * @param file_manager File storage component
   * @param system_monitor System monitoring component
   * @param tcp_server TCP server component (can be nullptr)
   * 
   * @note Takes ownership of analyzer, file manager, monitor, and server
   */
  Application(std::shared_ptr<DataSource> data_source,
              std::shared_ptr<RingBuffer<Sample>> buffer_raw,
              std::shared_ptr<RingBuffer<Sample>> buffer_classified,
              std::unique_ptr<ECGAnalyzer> ecg_analyzer,
              std::unique_ptr<FileManager> file_manager,
              std::unique_ptr<SystemMonitor> system_monitor,
              std::unique_ptr<TCPFileServer> tcp_server);

  /**
   * @brief Deleted assignment operator - Application is non-copyable.
   */
  Application(const Application&) = delete;

  /**
   * @brief Destructor - ensures graceful shutdown.
   */
  Application& operator=(const Application&) = delete;
  ~Application();

  /**
   * @brief Initializes all components and prepares for acquisition.
   * 
   * Validates data source availability, initializes file manager,
   * and optionally starts TCP server. Must be called before Run().
   * 
   * @return true if all components initialized successfully, false on error
   */
  bool Start();

  /**
   * @brief Main application loop - coordinates all threads.
   * 
   * Starts acquisition, analysis, and file writing threads.
   * Waits for completion or shutdown signal. Handles both normal
   * completion and signal-triggered graceful shutdown.
   * 
   * @note Blocks until acquisition completes or shutdown requested
   */
  void Run();

  /**
   * @brief Stops application and all components.
   * 
   * Signals shutdown to all threads and waits for completion.
   * Can be called from signal handler context.
   */
  void Stop();

  /**
   * @brief Checks if application is currently running.
   * 
   * @return true if acquisition is active, false otherwise
   */
  bool Running() const {return running_.load();}

  /**
   * @brief Requests application shutdown.
   * 
   * Called by signal handler on SIGINT/SIGTERM. Initiates
   * graceful shutdown sequence preserving all acquired data.
   * 
   * @note Thread-safe, can be called from signal handler
   */
  void RequestShutdown();

  void set_acquisition_duration(std::chrono::seconds duration);
  
  private:
  /**
   * @brief Data acquisition thread function.
   * 
   * Reads samples from data source at configured rate (250 Hz default)
   * and adds to raw buffer. Implements timing correction to maintain
   * accurate sample rate. Runs until duration expires or shutdown requested.
   * 
   * @note Uses sleep_until for precise timing, auto-corrects for delays
   */
  void AcquisitionLoop();

  /**
   * @brief Performs coordinated shutdown of all components.
   * 
   * Shutdown sequence:
   * 1. Stop acquisition immediately
   * 2. Process remaining ECG data
   * 3. Flush all buffers to disk
   * 4. Send files via TCP if client connected
   * 5. Close all resources
   * 
   * @note Ensures no data loss during signal-triggered shutdown
   */
  void GracefulShutdown();
};

#endif
