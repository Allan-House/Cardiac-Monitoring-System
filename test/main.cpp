#include "ads1115.h"
#include <iostream>

bool test_config_register_memory(ADS1115& ads1115);
bool test_config_register_hardware(ADS1115& ads1115);

int main() {
  std::cout << "[----------] Running ADS1115 tests" << std::endl;
  ADS1115 ads1115;

  std::cout << "[ RUN      ] test_config_register_memory" << std::endl;
  if (test_config_register_memory(ads1115)) {
    std::cout << "[       OK ] test_config_register_memory" << std::endl;
  }
  else {
    std::cout << "[  FAILED  ]" << std::endl;
  }

  std::cout << "[ RUN      ] test_config_register_hardware" << std::endl;
  if (test_config_register_hardware(ads1115)) {
    std::cout << "[       OK ] test_config_register_hardware" << std::endl;
  } else {
    std::cout << "[  FAILED  ] test_config_register_hardware" << std::endl;
  }

  return 0;
}

bool test_config_register_memory(ADS1115& ads1115) { 
  uint16_t expected_reg_value {0x4483};
  uint16_t actual_reg_value {ads1115.get_config_register()};
  
  std::cout << "    Expected: 0x" << std::hex << expected_reg_value << std::endl;
  std::cout << "    Actual:   0x" << std::hex << actual_reg_value << std::endl;
  
  return actual_reg_value == expected_reg_value;
}

bool test_config_register_hardware(ADS1115& ads1115) {
  if (!ads1115.Init()) {
    std::cout << "    Hardware not available (Init failed)" << std::endl;
    return false;
  }
  
  uint16_t hardware_value {ads1115.ReadConfigRegisterFromHardware()};
  
  if (hardware_value == 0xFFFF) {
      std::cout << "    Hardware not available (Read failed)" << std::endl;
      return false;
  }
  
  uint16_t memory_value {ads1115.get_config_register()};
  
  std::cout << "    Memory:   0x" << std::hex << memory_value << std::endl;
  std::cout << "    Hardware: 0x" << std::hex << hardware_value << std::endl;
  
  return hardware_value == memory_value;
}
