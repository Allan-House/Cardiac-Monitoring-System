#ifndef DATA_SOURCE_H_
#define DATA_SOURCE_H_

class DataSource {
  public:
  virtual ~DataSource() = default;
  virtual float ReadVoltage() = 0;
  virtual bool Available() const = 0;
};

#endif