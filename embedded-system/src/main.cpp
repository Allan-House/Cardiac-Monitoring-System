#include <iostream>
#include <wiringPi.h>

#define	LED	0

int main() {
  std::cout << "====================================" << std::endl;
  std::cout << "Welcome to Cardiac Monitoring System" << std::endl;
  std::cout << "====================================" << std::endl;

  std::cout << "Raspberry Pi blink" << std::endl;
  wiringPiSetup();
  pinMode(LED, OUTPUT);

  while (true) {
      digitalWrite (LED, HIGH);
    delay (500);
    digitalWrite (LED, LOW);
    delay (500);
  }
  
  return 0;
}