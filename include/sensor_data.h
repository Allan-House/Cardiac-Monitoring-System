#ifndef SENSOR_DATA_H_
#define SENSOR_DATA_H_

#include "ads1115.h"
#include "data_source.h"
#include <memory>
#include <optional>

class SensorData : public DataSource {
  private:
  std::shared_ptr<ADS1115> ads1115_;

  public:
  SensorData(std::shared_ptr<ADS1115> ads1115);
  
  std::optional<float> ReadVoltage() override;
  bool Available() const override;
};

#endif
