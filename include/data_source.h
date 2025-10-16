#ifndef DATA_SOURCE_H_
#define DATA_SOURCE_H_

#include <optional>

/**
 * @brief Abstract interface for ECG data sources.
 * 
 * Defines a common interface for acquiring voltage readings from
 * different sources (hardware sensors, file playback, simulators).
 * All data sources in the cardiac monitoring system must implement
 * this interface to ensure compatibility with the acquisition pipeline.
 */
class DataSource {
  public:
  virtual ~DataSource() = default;
  
  /**
   * @brief Reads a single voltage sample from the data source.
   * 
   * @return Voltage value in volts if successful, std::nullopt on error
   *         (e.g., hardware failure, end of file, unavailable source)
   * 
   * @note Implementations should handle their own timing/sampling rate
   *       internally if applicable.
   */
  virtual std::optional<float> ReadVoltage() = 0;
  
  /**
   * @brief Checks if the data source is available for reading.
   * 
   * @return true if the source is initialized and ready to provide data,
   *         false if the source is unavailable, not initialized, or has
   *         reached end of data (for file sources without loop)
   * 
   * @note Should be called before attempting to read data.
   */  
  virtual bool Available() const = 0;
};

#endif
