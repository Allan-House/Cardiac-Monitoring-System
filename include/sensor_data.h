#ifndef SENSOR_DATA_H_
#define SENSOR_DATA_H_

#include <memory>
#include <optional>

#include "ads1115.h"
#include "data_source.h"

/**
 * @brief Hardware data source implementation for ADS1115 ADC.
 * 
 * Provides real-time ECG data acquisition from ADS1115 connected
 * via I2C to the Raspberry Pi. Wraps the ADS1115 driver to conform
 * to the DataSource interface.
 * 
 * @note Used only in hardware builds (Raspberry Pi target)
 */
class SensorData : public DataSource {
  private:
  std::shared_ptr<ADS1115> ads1115_;

  public:
  /**
   * @brief Constructs sensor data source with ADS1115 driver.
   * 
   * @param ads1115 Shared pointer to configured ADS1115 instance
   * 
   * @note Initializes the ADS1115 hardware during construction
   */
  SensorData(std::shared_ptr<ADS1115> ads1115);

  /**
   * @brief Reads current voltage from ADS1115.
   * 
   * @return Voltage reading in volts, or std::nullopt on hardware error
   * 
   * @note This is a blocking call that waits for ADC conversion
   */
  std::optional<float> ReadVoltage() override;
  
  /**
   * @brief Checks if hardware is initialized and ready.
   * 
   * @return true if ADS1115 is initialized, false otherwise
   */
  bool Available() const override;
};

#endif
