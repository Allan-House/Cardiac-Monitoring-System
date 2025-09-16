#include "sensor_data.h"
#include "ads1115.h"
#include <memory>
#include <utility>

SensorData::SensorData(std::unique_ptr<ADS1115> ads1115)
  : ads1115_ {std::move(ads1115)}
{
  // Empty constructor.
}

float SensorData::ReadVoltage() {
  return ads1115_->ReadVoltage();
}

bool SensorData::Available() const {
  return ads1115_ != nullptr;
}
