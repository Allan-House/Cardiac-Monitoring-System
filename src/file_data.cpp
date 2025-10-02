#include "data_source.h"
#include "file_data.h"
#include "logger.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

FileData::FileData(const std::string& filename, float voltage_range, bool loop)
: voltage_range_ {voltage_range},
  loop_playback_ {loop}
{
  if (!LoadSamples(filename)) {
    LOG_ERROR("Failed to load samples from file: %s", filename.c_str());
    return;
  }

  if (samples_.empty()) {
    LOG_ERROR("No samples found in file: %s", filename.c_str());
    return;
  }

  initialized_ = true;
  LOG_SUCCESS("Loaded %zu samples from %s", samples_.size(), filename.c_str());
}


FileData::~FileData() {
  if (file_stream_.is_open()) {
    file_stream_.close();
  }
}


bool FileData::LoadSamples(const std::string& filename) {
  file_stream_.open(filename, std::ios::binary);

  if (!file_stream_.is_open()) {
    LOG_ERROR("Failed to open file: %s", filename);
    return false;
  }

  samples_.clear();

  int16_t raw_value;
  int64_t timestamp_us;

  while (file_stream_.read(reinterpret_cast<char*>(&raw_value),
        sizeof(raw_value)) &&
        file_stream_.read(reinterpret_cast<char*>(&timestamp_us),
        sizeof(timestamp_us))) {
    FileDataSample sample;
    sample.raw_value = raw_value;
    sample.timestamp_us = timestamp_us;
    sample.voltage = ConvertToVoltage(raw_value);
    
    samples_.push_back(sample);
  }

  file_stream_.close();
  return !samples_.empty();
}


float FileData::ConvertToVoltage(int16_t raw_value) const {
  return (raw_value * voltage_range_) / 32768.0f;
}


bool FileData::Initialized() const {
  return initialized_;
}


float FileData::ReadVoltage() {
  // TODO (allan): padronizar erro de leitura.
  if (!initialized_ || !Available()) {
    return 0.0f;
  }

  float voltage {samples_.at(current_index_).voltage};
  current_index_++;

  if (current_index_ >= samples_.size()) {
    if (loop_playback_) {
      Reset();
    } else {
      current_index_ = samples_.size(); // Stay at end;
    }
  }

  return voltage;
}


bool FileData::Available() const {
  if (!initialized_) {
    return false;
  }

  if (loop_playback_) {
    return !samples_.empty();
  }

  return current_index_ < samples_.size();
}


void FileData::Reset() {
  current_index_ = 0;
}


bool FileData::End() const {
  return current_index_ >= samples_.size();
}
