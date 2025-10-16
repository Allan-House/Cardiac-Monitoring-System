#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "ecg_analyzer.h"
#include "ring_buffer.h"

/**
 * @brief Manages persistent storage of ECG data to disk.
 * 
 * Handles dual-format file writing (binary and CSV) with buffered I/O
 * for efficient disk operations. Runs in a separate thread to prevent
 * blocking the data acquisition pipeline. Automatically creates output
 * directories and generates timestamped filenames.
 * 
 * Binary format: {int16_t raw_value, int64_t timestamp_us} per sample
 * CSV format: timestamp_us, voltage, classification columns
 */
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
  /**
   * @brief Constructs file manager with specified parameters.
   * 
   * @param buffer Shared pointer to ring buffer containing samples to write
   * @param base_filename Base name for output files (timestamp will be appended)
   * @param write_interval Interval between disk write operations (default: 200ms)
   * 
   * @note Output files are created in data/processed/ directory
   */
  FileManager(std::shared_ptr<RingBuffer<Sample>> buffer,
              const std::string& base_filename,
              std::chrono::milliseconds write_intervalo);
  
  /**
   * @brief Initializes file streams and writes CSV header.
   * 
   * Creates output files with timestamped names and opens streams.
   * Must be called before Run().
   * 
   * @return true if files opened successfully, false on error
   */
  bool Init();

  /**
   * @brief Starts the file writing thread.
   * 
   * Begins periodic writing of buffered samples to disk.
   */
  void Run();

  /**
   * @brief Stops writing thread and closes files.
   * 
   * Waits for thread to finish and ensures all data is flushed.
   */
  void Stop();

  /**
   * @brief Main loop for writing thread.
   * 
   * Periodically writes available samples from buffer to disk files.
   * Continues until stopped, then flushes remaining data.
   */
  void WritingLoop();

  /**
   * @brief Closes all file streams.
   */
  void Close();

  private:
  /**
   * @brief Writes CSV file header row.
   */
  void WriteCSVHeader();
  
  /**
   * @brief Writes batch of available samples from buffer.
   * 
   * Consumes up to 100 samples from buffer and writes to both files.
   */
  void WriteAvailableData();

  /**
   * @brief Writes single sample to both binary and CSV files.
   * 
   * @param sample Sample to write
   * 
   * @note Normalizes timestamps relative to first sample
   */
  void WriteSample(const Sample& sample);

  /**
   * @brief Flushes all remaining samples from buffer.
   * 
   * Called during shutdown to ensure no data loss.
   */
  void FlushRemainingData();

  /**
   * @brief Creates output directory structure.
   * 
   * Ensures data/processed/ and data/raw/ directories exist.
   */
  void CreateDirectories();

  /**
   * @brief Generates timestamp string for filenames.
   * 
   * @return Timestamp in YYYYMMDD_HHMMSS format
   */
  std::string GenerateTimestamp();
};

#endif
