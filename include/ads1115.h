#ifndef ADS_1115_
#define ADS_1115_

#include <cstdint>
#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>

namespace ads1115_constants {
  enum class Address : uint8_t {
    kGND = 0x48,
    kVDD = 0x49,
    kSDA = 0x4A,
    kSCL = 0x4B
  };

  enum class Register : uint8_t {
    kConversion   = 0x00,
    kConfig       = 0x01,
    kLOThreshold  = 0x02,
    kRHIThreshold = 0x03
  };

  enum class OpStatus : uint16_t {
    kStartSingle = 0x8000,
    kBusy        = 0x0000,
    kNotBusy     = 0x8000
  };

  enum class Mux : uint16_t {
    kA0_A1  = 0x0000,
    kA0_A3  = 0x1000,
    kA1_A3  = 0x2000,
    kA2_A3  = 0x3000,
    kA0_GND = 0x4000,
    kA1_GND = 0x5000,
    kA2_GND = 0x6000,
    kA3_GND = 0x7000
  };

  enum class Gain : uint16_t {
    kFSR_6_144V = 0x0000,
    kFSR_4_096V = 0x0200,
    kFSR_2_048V = 0x0400,
    kFSR_1_024V = 0x0600,
    kFSR_0_512V = 0x0800,
    kFSR_0_256V_A = 0x0A00,
    kFSR_0_256V_B = 0x0C00,
    kFSR_0_256V_C = 0x0E00
  };

  enum class DataRate : uint16_t {
    kSPS_8   = 0x0000,
    kSPS_16  = 0x0020,
    kSPS_32  = 0x0040,
    kSPS_64  = 0x0060,
    kSPS_128 = 0x0080, // default
    kSPS_250 = 0x00A0,
    kSPS_475 = 0x00C0,
    kSPS_860 = 0x00E0
  };

  enum class Mode : uint16_t {
    kContinuous = 0x0000,
    kSingle     = 0x0100 // default
  };

  enum class ComparatorMode : uint16_t {
    kTraditional = 0x0000,
    kWindow      = 0x0010
  };
}

class ADS1115 {
  static constexpr float kErrorVoltage = -999.0f;

  private:
  uint16_t config_register_;
  uint8_t i2c_address_;
  int i2c_fd_;
  bool initialized_;
  float voltage_range_;

  uint16_t ReadRegister(uint8_t reg);
  bool WriteRegister(uint8_t reg, uint16_t value);
  void CalculateVoltageRange();
  float ConvertToVoltage(int16_t raw_value);
  
  public:
  ADS1115(uint8_t address = static_cast<uint8_t>(ads1115_constants::Address::kGND));
  ~ADS1115();
  
  bool Init();

  int16_t ReadRawADC();
  float ReadVoltage();
  uint16_t ReadConfigRegisterFromHardware();
  bool VerifyConfigRegister();
  
  void set_data_rate(ads1115_constants::DataRate data_rate);
  void set_mode(ads1115_constants::Mode mode);
  void set_gain(ads1115_constants::Gain gain);
  void set_mux(ads1115_constants::Mux mux);

  uint16_t get_config_register() {return config_register_;}
};

#endif
