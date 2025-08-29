#include "ads1115.h"
#include <unistd.h>
#include <wiringPiI2C.h>

ADS1115::ADS1115(uint8_t address) :
config_register_ {static_cast<uint16_t>(ads1115_constants::Mux::kA0_GND)       |
                  static_cast<uint16_t>(ads1115_constants::Gain::kFSR_2_048V)  | 
                  static_cast<uint16_t>(ads1115_constants::Mode::kContinuous)  |
                  static_cast<uint16_t>(ads1115_constants::DataRate::kSPS_128) |
                  0x0003},
i2c_address_ {address},
i2c_fd_ {-1},
initialized_ {false},
voltage_range_ {2.048f}
{
  // Empty constructor.
}

ADS1115::~ADS1115() {
  if (i2c_fd_ >= 0) {
    close(i2c_fd_);
  }
}

uint16_t ADS1115::ReadRegister(uint8_t reg) {
  if (i2c_fd_ < 0) {
    std::cerr << "I2C not initialized!" << std::endl;
    return 0xFFFF;
  }
  
  int result = wiringPiI2CReadReg16(i2c_fd_, reg);
  
  if (result < 0) {
    std::cerr << "Error reading register 0x" << std::hex
              << static_cast<int>(reg) << std::endl;
    return 0xFFFF;
  }
  
  // WiringPi returns in little-endian, but ADS1115 uses big-endian.
  return ((result & 0xFF) << 8) | ((result & 0xFF00) >> 8);
}

bool ADS1115::WriteRegister(uint8_t reg, uint16_t value) {
  if (i2c_fd_ < 0) {
    std::cerr << "I2C not initialized!" << std::endl;
    return false;
  }
  
  // WiringPiWriteReg16 expects little-endian, but ADS1115 expects big-endian.
  int result = wiringPiI2CWriteReg16(i2c_fd_, reg, ((value & 0xFF) << 8) |
                                                   ((value & 0xFF00) >> 8));
  
  if (result < 0) {
    std::cerr << "Error writing register 0x" << std::hex
              << static_cast<int>(reg) << std::endl;
    return false;
  }
  
  return true;
}

void ADS1115::CalculateVoltageRange() {
  uint16_t gain = config_register_ & 0x0E00;
  switch (static_cast<ads1115_constants::Gain>(gain)) {
    case ads1115_constants::Gain::kFSR_6_144V: voltage_range_ = 6.144f; break;
    case ads1115_constants::Gain::kFSR_4_096V: voltage_range_ = 4.096f; break;
    case ads1115_constants::Gain::kFSR_2_048V: voltage_range_ = 2.048f; break;
    case ads1115_constants::Gain::kFSR_1_024V: voltage_range_ = 1.024f; break;
    case ads1115_constants::Gain::kFSR_0_512V: voltage_range_ = 0.512f; break;
    case ads1115_constants::Gain::kFSR_0_256V_A: voltage_range_ = 0.256f; break;
    case ads1115_constants::Gain::kFSR_0_256V_B: voltage_range_ = 0.256f; break;
    case ads1115_constants::Gain::kFSR_0_256V_C: voltage_range_ = 0.256f; break;
    default: voltage_range_ = 2.048f; break;
    }
}

float ADS1115::ConvertToVoltage(int16_t raw_value) {
  return (raw_value * voltage_range_) / 32768.0f;
}

bool ADS1115::Init() {

  if (initialized_) {
    std::cout << "ADS1115 already initialized!" << std::endl;
    return true;
  }

  if (i2c_fd_ >= 0) {
    close(i2c_fd_);
    i2c_fd_ = -1;
  }

  if (wiringPiSetup() < 0) {
    std::cerr << "Error initializing WiringPi!" << std::endl;
    return false;
  }
  
  i2c_fd_ = wiringPiI2CSetup(i2c_address_);
  if (i2c_fd_ < 0) {
    std::cerr << "Error initializing I2C with ADS1115!" << std::endl;
    return false;
  }

  uint8_t reg {static_cast<uint8_t>(ads1115_constants::Register::kConfig)};
  bool success = WriteRegister(reg, config_register_);
  if (!success) {
    std::cerr << "Error configuring ADS1115!" << std::endl;
    return false;
  }
  
  std::cout << "ADS1115 initialized successfully!" << std::endl;
  initialized_ = true;
  return true;
}

int16_t ADS1115::ReadRawADC() {
  if (!initialized_) {
    std::cerr << "ADS1115 not initialized! Call Init() first." << std::endl;
    return INT16_MIN;
  }

  uint16_t raw_data {ReadRegister(static_cast<uint8_t>(
                                  ads1115_constants::Register::kConversion))};
  return static_cast<int16_t>(raw_data);
}

float ADS1115::ReadVoltage() {
  int16_t raw_value {ReadRawADC()};

  if (raw_value == INT16_MIN) {
    return kErrorVoltage;
  }

  return ConvertToVoltage(raw_value);
}

uint16_t ADS1115::ReadConfigRegisterFromHardware() {
  if (!initialized_) {
    std::cerr << "ADS1115 not initialized! Call Init() first." << std::endl;
    return 0xFFFF;
  }
  
  return ReadRegister(static_cast<uint8_t>(ads1115_constants::Register::kConfig));
}

bool ADS1115::VerifyConfigRegister() {
  uint16_t hardware_value {ReadConfigRegisterFromHardware()};
  
  if (hardware_value == 0xFFFF) {
    return false;
  }
  
  bool match {(hardware_value == config_register_)};
  
  if (!match) {
    std::cout << "Config register mismatch!" << std::endl;
    std::cout << "  Memory:   0x" << std::hex << config_register_ << std::endl;
    std::cout << "  Hardware: 0x" << std::hex << hardware_value << std::endl;
  }
  
  return match;
}

// Setters
void ADS1115::set_data_rate(ads1115_constants::DataRate data_rate) {
  config_register_ &= ~0x00E0;
  config_register_ |= static_cast<uint16_t>(data_rate);
  WriteRegister(static_cast<uint8_t>(ads1115_constants::Register::kConfig),
                config_register_);
}

void ADS1115::set_gain(ads1115_constants::Gain gain) {
  config_register_ &= ~0x0E00;
  config_register_ |= static_cast<uint16_t>(gain);
  CalculateVoltageRange();
  WriteRegister(static_cast<uint8_t>(ads1115_constants::Register::kConfig),
                config_register_);
}

void ADS1115::set_mode(ads1115_constants::Mode mode) {
  config_register_ &= ~0x0100;
  config_register_ |= static_cast<uint16_t>(mode);
  WriteRegister(static_cast<uint8_t>(ads1115_constants::Register::kConfig),
                config_register_);
}

void ADS1115::set_mux(ads1115_constants::Mux mux) {
  config_register_ &= ~0x7000;
  config_register_ |= static_cast<uint16_t>(mux);
  WriteRegister(static_cast<uint8_t>(ads1115_constants::Register::kConfig),
                config_register_);
}
