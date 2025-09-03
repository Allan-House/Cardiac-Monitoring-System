#include "file_manager.h"
#include "logger.h"
#include <filesystem>
#include <iostream>

FileManager::FileManager(std::shared_ptr<RingBuffer<Sample>> buffer,
                        const std::string& bin_filename,
                        const std::string& csv_filename,
                        std::chrono::milliseconds write_interval)
  : buffer_ {buffer},
    bin_filename_ {bin_filename},
    csv_filename_ {csv_filename},
    write_interval_ {write_interval}
{
  // Empty constructor
}

FileManager::~FileManager() {
  Close();
}

bool FileManager::Init() {
  LOG_INFO("Initializing FileManager with files: %s %s",
            bin_filename_.c_str(), csv_filename_.c_str());
  
  bin_stream_ = std::make_unique<std::ofstream>(bin_filename_,
                                                std::ios::out | 
                                                std::ios::binary |
                                                std::ios::trunc);

  csv_stream_ = std::make_unique<std::ofstream>(csv_filename_,
                                                std::ios::out |
                                                std::ios::trunc);

  if (!bin_stream_->is_open()) {
    LOG_ERROR("Failed to open binary file: %s", bin_filename_.c_str());
    return false;
  }

  if (!csv_stream_->is_open()) {
    LOG_ERROR("Failed to open CSV file: %s", csv_filename_.c_str());
    return false;
  }
  
  LOG_SUCCESS("Files opened successfully");
  WriteCSVHeader();
  
  return true;
}

void FileManager::WriterLoop(std::atomic<bool>& running) {
  auto next_write_time {std::chrono::steady_clock::now()};
  
  LOG_INFO("Starting files writing thread...");
  
  while (running) {
    std::this_thread::sleep_until(next_write_time);
    WriteAvailableData();
    next_write_time += write_interval_;
  }

  LOG_INFO("Stopping files writing thread...");
}

void FileManager::Close() {
  if ((csv_stream_ && csv_stream_->is_open()) ||
    (bin_stream_ && bin_stream_->is_open())) {
    FlushRemainingData();
  }
  if (csv_stream_ && csv_stream_->is_open()) {csv_stream_->close();}
  if (bin_stream_ && bin_stream_->is_open()) {bin_stream_->close();}
  LOG_INFO("File closed. Total samples: %zu, Total bytes: %zu", 
            samples_written_, total_bytes_written_);
}

void FileManager::WriteCSVHeader() {
  if (csv_stream_ && csv_stream_->is_open()) {
    *csv_stream_ << "timestamp_us,voltage\n";
    csv_stream_->flush();
  } else {
    LOG_ERROR("Cannot write CSV header - file stream not open");
  }
}

void FileManager::WriteAvailableData() {
  if ((!csv_stream_ || !csv_stream_->is_open()) &&
    (!bin_stream_ || !bin_stream_->is_open())) {
    LOG_WARN("Neither CSV nor binary streams are open");
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
    if (csv_stream_ && csv_stream_->is_open()) csv_stream_->flush();
    if (bin_stream_ && bin_stream_->is_open()) bin_stream_->flush();
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
  
  // Binary
  if (bin_stream_ && bin_stream_->is_open()) {
    // Converts voltage to int16_t
    float scaled {sample.voltage * 32768.0f / 4.096f};
    if (scaled > 32767) {scaled = 32767;}
    if (scaled < -32768) {scaled = -32768;}
    int16_t raw_data = static_cast<int16_t>(scaled);

    //
    int64_t timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
                           sample.timestamp.time_since_epoch()).count();

    bin_stream_->write(reinterpret_cast<char*>(&raw_data), sizeof(raw_data));
    bin_stream_->write(reinterpret_cast<char*>(&timestamp_us), sizeof(timestamp_us));
    total_bytes_written_ += sizeof(raw_data) + sizeof(timestamp_us);
  }

  // CSV
  if (csv_stream_ && csv_stream_->is_open()) {
    std::string line {std::to_string(normalized_timestamp) + "," + 
                      std::to_string(sample.voltage) + "\n"};
    *csv_stream_ << line;
    total_bytes_written_ += line.length();
  }
}

void FileManager::FlushRemainingData() {
  Sample sample;
  size_t final_count {0};

  if ((!csv_stream_ || !csv_stream_->is_open()) &&
    (!bin_stream_ || !bin_stream_->is_open())) {
    LOG_WARN("Neither CSV nor binary streams are open");
    return;
  }

  while (!buffer_->Empty()) {
    sample = buffer_->Consume();
    WriteSample(sample);
    samples_written_++;
    final_count++;
  }

  if (final_count > 0) {
    if (csv_stream_ && csv_stream_->is_open()) csv_stream_->flush();
    if (bin_stream_ && bin_stream_->is_open()) bin_stream_->flush();
    LOG_INFO("Flushed %zu remaining samples", final_count);
  }
}