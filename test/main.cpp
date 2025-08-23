#include "ads1115.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

bool test_config_register_memory(ADS1115& ads1115);
bool test_config_register_hardware(ADS1115& ads1115);
bool test_voltage_reading(ADS1115& ads1115);

// TODO (allan): Padronizar os prints e usar o logger.
int main() {
  // std::cout << "[----------] Running ADS1115 tests" << std::endl;
  ADS1115 ads1115;

  /*
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
  */
  if (test_voltage_reading(ads1115)) {
    std::cout << "[       OK ] ADS1115 Voltage Reading Test" << std::endl;
  }
  else {
    std::cout << "[  FAILED  ]" << std::endl;
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

bool test_voltage_reading(ADS1115& ads1115) {
  std::cout << "=== ADS1115 Voltage Reading Test ===" << std::endl;
  std::cout << "Connect A0 to 3.3V and press Enter to start..." << std::endl;
  std::cin.get();

  if (!ads1115.Init()) {
    std::cout << "ERROR: Failed to initialize ADS1115" << std::endl;
    return false;
  }

  ads1115.set_gain(ads1115_constants::Gain::kFSR_4_096V);
  ads1115.ReadVoltage(); // Descarta primeira leitura
  std::this_thread::sleep_for(std::chrono::milliseconds(100));


  std::cout << "Configuration applied (Range: ±4.096V, Channel: A0)" << std::endl;
  std::cout << "Expected reading: ~3.30V" << std::endl;
  std::cout << "Press Ctrl+C to stop..." << std::endl;
  std::cout << std::endl;

  float voltage {0.0f};
  bool test_result {true};
  for (size_t i = 0; i < 10; i++) {
    voltage = ads1115.ReadVoltage();

    std::string status;
    if (voltage == -999.0f) {
      status = "ERROR";
    } else if (voltage >= 3.20f && voltage <= 3.40f) {
      status = "OK";
    } else if (voltage >= 2.90f && voltage <= 3.70f) {
      status = "WARN";
    } else {
      status = "OUT_OF_RANGE";
    }

    test_result &= status == "OK";
    std::cout << "Reading #" << std::setw(4) << std::dec << (i + 1)
              << " | A0: " << std::fixed << std::setprecision(3) << std::setw(6) << voltage 
              << "V | " << status << std::endl;
      
    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 2 Hz
  }
  return test_result;
}
