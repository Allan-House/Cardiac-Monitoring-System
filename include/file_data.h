#ifndef FILE_DATA_H_
#define FILE_DATA_H_

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "config.h"
#include "data_source.h"

/**
 * @brief Structure representing a single ECG sample from file.
 * 
 * Stores raw ADC value, timestamp, and converted voltage for
 * efficient playback of recorded ECG data.
 */
struct FileDataSample {
  int16_t raw_value;
  int64_t timestamp_us;
  float voltage;
};

/**
 * @brief File-based data source for ECG signal playback.
 * 
 * Loads and plays back previously recorded ECG data from binary files.
 * Supports both single-pass and continuous loop playback modes.
 * Used primarily for development, testing, and offline analysis.
 * 
 * File format: Sequential records of {int16_t voltage, int64_t timestamp}
 */
class FileData : public DataSource {
  private:
  std::ifstream file_stream_;
  std::vector<FileDataSample> samples_;
  size_t current_index_ {0};
  float voltage_range_ {config::kVoltageRange};
  bool loop_playback_ {true};
  bool initialized_ {false};
  
  public:
  /**
   * @brief Constructs file data source and loads samples.
   * 
   * @param filename Path to binary ECG data file
   * @param voltage_range ADC voltage range for conversion (default: 4.096V)
   * @param loop Enable continuous playback loop (default: true)
   * 
   * @note File is loaded entirely into memory during construction
   */
  explicit FileData(const std::string& filename, 
                   float voltage_range = config::kVoltageRange, 
                   bool loop = true);
  
  /**
   * @brief Destructor - closes file stream if open.
   */
  ~FileData();
  
  /**
   * @brief Checks if file was successfully loaded.
   * 
   * @return true if samples were loaded successfully, false otherwise
   */
  bool Initialized() const;
  
  /**
   * @brief Reads next voltage sample from loaded data.
   * 
   * @return Voltage in volts, or std::nullopt if unavailable
   * 
   * @note In loop mode, automatically resets to beginning after last sample
   */
  std::optional<float> ReadVoltage() override;
  
  /**
   * @brief Checks if data is available for reading.
   * 
   * @return true if samples are available (always true in loop mode),
   *         false if at end of data in single-pass mode
   */
  bool Available() const override;
  
  // File data management

  /**
   * @brief Resets playback to first sample.
   */
  void Reset();

  /**
   * @brief Checks if playback has reached end of data.
   * 
   * @return true if at or past last sample, false otherwise
   */
  bool End() const;

  size_t get_current_index() const {return current_index_;}
  size_t get_total_samples() const {return samples_.size();}
  
  void set_loop_playback(bool loop) {loop_playback_ = loop;}

  private:

  /**
   * @brief Loads all samples from binary file into memory.
   * 
   * @param filename Path to binary ECG file
   * @return true if loading successful, false on error
   */
  bool LoadSamples(const std::string& filename);
  
  /**
   * @brief Converts raw ADC value to voltage.
   * 
   * @param raw_value 16-bit signed ADC value
   * @return Voltage in volts based on configured range
   */
  float ConvertToVoltage(int16_t raw_value) const;
};

#endif
