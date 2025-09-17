#ifndef FILE_DATA_H_
#define FILE_DATA_H_

#include "data_source.h"
#include <fstream>
#include <string>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <vector>

struct FileDataSample {
  int16_t raw_value;
  int64_t timestamp_us;
  float voltage;
};

class FileData : public DataSource {
  private:
  std::ifstream file_stream_;
  std::vector<FileDataSample> samples_;
  size_t current_index_ {0};
  float voltage_range_ {4.096f};
  bool loop_playback_ {true};
  bool initialized_ {false};
  
  public:
  explicit FileData(const std::string& filename, 
                   float voltage_range = 4.096f, 
                   bool loop = true);
  ~FileData();
  
  bool Initialized() const;
  
  float ReadVoltage() override;
  bool Available() const override;
  
  // File data management
  void Reset();
  bool End() const;
  size_t get_total_samples() const {return samples_.size();}
  size_t get_current_index() const {return current_index_;}
  void set_loop_playback(bool loop) {loop_playback_ = loop;}

  private:
  bool LoadSamples(const std::string& filename);
  float ConvertToVoltage(int16_t raw_value) const;
};

#endif