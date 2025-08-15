#include "ads1115.h"

ADS1115::ADS1115(uint8_t address) :
i2c_address_ {address},
i2c_fd_ {-1},
config_register_ {static_cast<uint16_t>(ads1115_constants::Mux::kA0_GND)       |
                  static_cast<uint16_t>(ads1115_constants::Gain::kFSR_2_048V)  |
                  static_cast<uint16_t>(ads1115_constants::Mode::kSingle)      |
                  static_cast<uint16_t>(ads1115_constants::DataRate::kSPS_128) | 
                  0x0003}
{
  CalculateVoltageRange();
}

void ADS1115::CalculateVoltageRange() {
  uint16_t gain = config_register_ & 0x0E00;
  switch (static_cast<ads1115_constants::Gain>(gain)) {
    case ads1115_constants::Gain::kFSR_6_144V: voltage_range_ = 6.144f; break;
    case ads1115_constants::Gain::kFSR_4_096V: voltage_range_ = 4.096f; break;
    case ads1115_constants::Gain::kFSR_2_048V: voltage_range_ = 2.048f; break;
    case ads1115_constants::Gain::kFSR_1_024V: voltage_range_ = 1.024f; break;
    case ads1115_constants::Gain::kFSR_0_512V: voltage_range_ = 0.512f; break;
    case ads1115_constants::Gain::kFSR_0_256V: voltage_range_ = 0.256f; break;
    default: voltage_range_ = 2.048f; break;
    }
}

uint16_t ADS1115::ReadRegister(uint8_t reg) {
  if (i2c_fd_ < 0) {
    std::cerr << "I2C não inicializado!" << std::endl;
    return 0;
  }
    
  // WiringPi lê 16 bits.
  int result = wiringPiI2CReadReg16(i2c_fd_, reg);
    
 if (result < 0) {
    // TODO (allan): static_cast?
    std::cerr << "Erro ao ler registrador 0x" << std::hex << (int)reg << std::endl;
    return 0;
  }
    
  // WiringPi retorna em little-endian, mas ADS1115 usa big-endian.
  // Então precisamos fazer o swap dos bytes.
  return ((result & 0xFF) << 8) | ((result & 0xFF00) >> 8);
}

bool   ADS1115::WriteRegister(uint8_t reg, uint16_t value) {
  if (i2c_fd_ < 0) {
    std::cerr << "I2C não inicializado!" << std::endl;
    return false;
  }
  
  // WiringPi escreve 16 bits em big-endian automaticamente
  int result = wiringPiI2CWriteReg16(i2c_fd_, reg, ((value & 0xFF) << 8) |
                                     ((value & 0xFF00) >> 8));
  
  if (result < 0) {
    // TODO (allan): static_cast?
    std::cerr << "Erro ao escrever registrador 0x" << std::hex << (int)reg << std::endl;
    return false;
  }

  return true;
}

float ADS1115::ConvertToVoltage(int16_t raw_value) {
  return (raw_value * voltage_range_) / 32767.0f;
}

bool ADS1115::Init() {
  // TODO (allan): tratamento de erro?
  wiringPiSetup();
  
  i2c_fd_ = wiringPiI2CSetup(i2c_address_);
  if (i2c_fd_ < 0) {
    std::cerr << "Erro ao inicializar I2C com ADS1115!" << std::endl;
    return false;
  }
  uint8_t reg {static_cast<uint8_t>(ads1115_constants::Register::kConfig)};
  bool success = WriteRegister(reg, config_register_);
  if (!success) {
    std::cerr << "Erro ao configurar ADS1115!" << std::endl;
    return false;
  }
  
  std::cout << "ADS1115 inicializado com sucesso!" << std::endl;
  return true;
}

void ADS1115::set_data_rate(ads1115_constants::DataRate data_rate) {
  config_register_ &= ~0x00E0;  // Limpa bits de data rate
  config_register_ |= static_cast<uint16_t>(data_rate);
}

void ADS1115::set_gain(ads1115_constants::Gain gain) {
  config_register_ &= ~0x0E00; // Limpa bits de ganho
  config_register_ |= static_cast<uint16_t>(gain);;
  CalculateVoltageRange();
}

void ADS1115::set_mode(ads1115_constants::Mode mode) {
  config_register_ &= ~0x0100;  // Limpa bits de modo
  config_register_ |= static_cast<uint16_t>(mode);
}
