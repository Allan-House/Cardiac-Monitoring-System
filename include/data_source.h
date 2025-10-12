#ifndef DATA_SOURCE_H_
#define DATA_SOURCE_H_

#include <optional>

class DataSource {
  public:
  virtual ~DataSource() = default;
  virtual std::optional<float> ReadVoltage() = 0;
  virtual bool Available() const = 0;
};

#endif
