#ifndef FILEMANAGER_H
#define FILEMANAGER_H
#include <memory>
#include <fstream>
#include <chrono>
#include <atomic>
#include <thread>
#include <filesystem>
#include "ring_buffer.h"
#include "ecg_analyzer.h"

class FileManager {
  private:
  // External dependencies
  std::shared_ptr<RingBuffer<Sample>> buffer_;
  std::unique_ptr<std::ofstream> bin_stream_;
  std::unique_ptr<std::ofstream> csv_stream_;

  // Configuration
  std::string bin_filename_;
  std::string csv_filename_;
  std::chrono::milliseconds write_interval_;

  // Runtime state
  std::optional<uint64_t> first_timestamp_;
  size_t samples_written_ {0};
  size_t total_bytes_written_ {0};
  std::atomic<bool> writing_ {false};
  std::thread writing_thread_;

  public:
  FileManager(std::shared_ptr<RingBuffer<Sample>> buffer,
              const std::string& base_filename,
              std::chrono::milliseconds write_intervalo);
  
  bool Init();
  void Run();
  void Stop();
  void WritingLoop();
  void Close();
  
  // Stats
  size_t get_samples_written() const {return samples_written_;}
  size_t get_bytes_written() const {return total_bytes_written_;}

  private:
  void WriteCSVHeader();
  void WriteAvailableData();
  void WriteSample(const Sample& sample);
  void FlushRemainingData();
  void CreateDirectories();
  std::string GenerateTimestamp();
};

#endif