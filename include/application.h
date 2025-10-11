#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "config.h"
#include "data_source.h"
#include "ecg_analyzer.h"
#include "file_manager.h"
#include "ring_buffer.h"
#include "system_monitor.h"
#include "tcp_file_server.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>

class Application {
  private:
  
  // External dependencies
  // TODO (allan): decidir quais devem ser unique_ptr e quais devem ser shared_ptr 
  std::shared_ptr<DataSource> data_source_;
  std::shared_ptr<RingBuffer<Sample>> buffer_raw_;
  std::shared_ptr<RingBuffer<Sample>> buffer_classified_;
  std::shared_ptr<ECGAnalyzer> ecg_analyzer_;
  std::shared_ptr<FileManager> file_manager_;
  std::shared_ptr<SystemMonitor> system_monitor_;
  std::shared_ptr<TCPFileServer> tcp_server_;

  // Thread management
  std::thread acquisition_thread_;

  // Synchronization
  std::atomic<bool> running_ {false};

  // Runtime configuration
  std::chrono::seconds acquisition_duration_ {70};

  public:
  Application(std::shared_ptr<DataSource> data_source,
              std::shared_ptr<RingBuffer<Sample>> buffer_raw,
              std::shared_ptr<RingBuffer<Sample>> buffer_classified,
              std::shared_ptr<ECGAnalyzer> ecg_analyzer,
              std::shared_ptr<FileManager> file_manager,
              std::shared_ptr<SystemMonitor> system_monitor,
              std::shared_ptr<TCPFileServer> tcp_server);
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  ~Application();

  // Main control functions
  bool Start();
  void Run();
  void Stop();

  // Status
  bool Running() const {return running_.load();}

  void set_acquisition_duration(std::chrono::seconds duration);
  
  private:
  // Thread functions
  void AcquisitionLoop();
};

#endif
