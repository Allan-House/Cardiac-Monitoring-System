#include <iostream>
#include <wiringPi.h>
#include "logger.h"

#define	LED	0

int main() {
  // Inicializa logger
  cardiac_logger::init("cardiac_system.log", cardiac_logger::Level::INFO);

  LOG_INFO("===================================");
  LOG_INFO("Cardiac Monitoring System Starting");
  LOG_INFO("===================================");

  LOG_INFO("Initializing Raspberry Pi GPIO...");
  wiringPiSetup();
  pinMode(LED, OUTPUT);
  LOG_INFO("GPIO initialized successfully");

  while (true) {
    digitalWrite (LED, HIGH);
    LOG_DEBUG("LED ON");
    delay (500);

    digitalWrite (LED, LOW);
    LOG_DEBUG("LED OFF");
    delay (500);
  }
  
  cardiac_logger::shutdown();
  return 0;
}