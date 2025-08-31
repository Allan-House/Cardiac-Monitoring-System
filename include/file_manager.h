#ifndef FILEMANAGER_H
#define FILEMANAGER_H
#include <memory>
#include <fstream>
#include <chrono>
#include <atomic>
#include "ring_buffer.h"
#include "ecg_analyzer.h"

class FileManager {
  private:
  // External dependencies
  std::shared_ptr<RingBuffer<Sample>> buffer_;
  std::unique_ptr<std::ofstream> file_stream_;

  // Configuration
  std::string filename_;
  std::chrono::milliseconds write_interval_;

  // Runtime state
  std::optional<uint64_t> first_timestamp_;
  size_t samples_written_{0};
  size_t total_bytes_written_{0};

  public:
  FileManager(std::shared_ptr<RingBuffer<Sample>> buffer, 
              const std::string& filename,
              std::chrono::milliseconds write_interval = std::chrono::milliseconds(100));
  
  ~FileManager();
  
  bool Init();
  void WriterLoop(std::atomic<bool>& running);
  void Close();
  
  // Stats
  size_t get_samples_written() const { return samples_written_; }
  size_t get_bytes_written() const { return total_bytes_written_; }

  private:
  void WriteHeader();
  void WriteAvailableData();
  void WriteSample(const Sample& sample);
  void FlushRemainingData();
};

#endif
