#include "file_manager.h"
#include "logger.h"
#include <filesystem>
#include <iostream>

FileManager::FileManager(std::shared_ptr<RingBuffer<Sample>> buffer, 
                        const std::string& filename,
                        std::chrono::milliseconds write_interval)
  : buffer_(buffer), filename_(filename), write_interval_(write_interval)
{
  // Empty constructor
}

FileManager::~FileManager() {
  Close();
}

bool FileManager::Init() {
  LOG_INFO("Initializing FileManager with file: %s", filename_.c_str());
  
  file_stream_ = std::make_unique<std::ofstream>(filename_, std::ios::out | std::ios::trunc);
  
  if (!file_stream_->is_open()) {
    LOG_ERROR("Failed to create/open file: %s", filename_.c_str());
    LOG_ERROR("Current working directory: %s", std::filesystem::current_path().c_str());
    return false;
  }
  
  LOG_INFO("File opened successfully: %s", filename_.c_str());
  WriteHeader();
  
  return true;
}

void FileManager::WriterLoop(std::atomic<bool>& running) {
  auto next_write_time {std::chrono::steady_clock::now()};
  
  LOG_TRACE("FileManager writer loop started");
  
  while (running) {
    std::this_thread::sleep_until(next_write_time);
    WriteAvailableData();
    next_write_time += write_interval_;
  }
  
  LOG_TRACE("FileManager writer loop finished");
}

void FileManager::Close() {
  if (file_stream_ && file_stream_->is_open()) {
    FlushRemainingData();
    file_stream_->close();
    LOG_INFO("File closed. Total samples: %zu, Total bytes: %zu", 
              samples_written_, total_bytes_written_);
  }
}

void FileManager::WriteHeader() {
  if (file_stream_ && file_stream_->is_open()) {
    *file_stream_ << "timestamp_us,voltage\n";
    file_stream_->flush();
    LOG_DEBUG("CSV header written");
  } else {
    LOG_ERROR("Cannot write header - file stream not open");
  }
}

void FileManager::WriteAvailableData() {
  if (!file_stream_ || !file_stream_->is_open()) {
    LOG_ERROR("WriteAvailableData: File stream not open");
    return;
  }

  Sample sample;
  size_t batch_count {0};
  const size_t max_batch_size {100};

  while (!buffer_->Empty() && batch_count < max_batch_size) {
    sample = buffer_->Consume();
    WriteSample(sample);
    batch_count++;
    samples_written_++;
  }
  
  if (batch_count > 0) {
    file_stream_->flush();
  }
}

void FileManager::WriteSample(const Sample& sample) {
  auto duration = sample.timestamp.time_since_epoch();
  auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
  
  // Normalize using the first timestamp
  if (!first_timestamp_) {
    first_timestamp_ = microseconds;
  }
  
  uint64_t normalized_timestamp = microseconds - first_timestamp_.value();
  
  std::string line {std::to_string(normalized_timestamp) + "," + 
                    std::to_string(sample.voltage) + "\n"};
  
  *file_stream_ << line;
  total_bytes_written_ += line.length();
}

void FileManager::FlushRemainingData() {
  if (!file_stream_ || !file_stream_->is_open()) {
  LOG_WARN("FlushRemainingData: File stream not open");
  return;
  }

  Sample sample;
  size_t final_count {0};
  
  while (!buffer_->Empty()) {
    sample = buffer_->Consume();
    WriteSample(sample);
    samples_written_++;
    final_count++;
  }

  if (final_count > 0) {
    file_stream_->flush();
    LOG_INFO("Flushed %zu remaining samples", final_count);
  }
}