#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#define LED_PIN 17      // GPIO 17 (physical pin 11)
#define BUTTON_PIN 27   // GPIO 27 (physical pin 13)

int test_gpio() {
    printf("\n=== GPIO TEST ===\n");
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pullUpDnControl(BUTTON_PIN, PUD_UP);
    printf("Pins configured:\n");

    printf("\nTesting digital outputs...\n");
    for(int i = 0; i < 5; i++) {
        printf("Turning LEDs on... (%d/5)\n", i+1);
        digitalWrite(LED_PIN, HIGH);
        usleep(500000); // 500ms
        
        printf("Turning LEDs off...\n");
        digitalWrite(LED_PIN, LOW);
        usleep(500000);
    }
    
    printf("\nTesting digital input (5 seconds)...\n");
    printf("Button state (1 = released, 0 = pressed): ");
    fflush(stdout);
    
    for(int i = 0; i < 50; i++) {
        int button_state = digitalRead(BUTTON_PIN);
        printf("%d ", button_state);
        fflush(stdout);
        usleep(100000); // 100ms
    }
    printf("\n");

    digitalWrite(LED_PIN, LOW);
    printf("GPIO test completed successfully!\n");
    return 0;
}

// Function to show system information
void show_system_info() {
    printf("=== SYSTEM INFORMATION ===\n");
    printf("WiringPi version: ");
    fflush(stdout);
    system("gpio -v | head -1");
    
    printf("\nPin mapping used in this test:\n");
    printf("WiringPi | GPIO | Physical Pin | Function\n");
    printf("---------|------|---------------|----------\n");
    printf("    0    |  17  |      11       | OUTPUT\n");
    printf("    2    |  27  |      13       | INPUT\n");
    printf("\nTo see complete mapping: gpio readall\n");
}

int main() {
    printf("=== WIRINGPI INSTALLATION TEST ===\n");   
    printf("Initializing WiringPi...\n");
    if(wiringPiSetupGpio() == -1) {
        fprintf(stderr, "ERROR: Failed to initialize WiringPi!\n");
        fprintf(stderr, "Check if WiringPi is installed correctly.\n");
        exit(1);
    }
    printf("WiringPi initialized successfully!\n");
    
    show_system_info();
    
    if(test_gpio() != 0) {
        fprintf(stderr, "ERROR in GPIO test!\n");
        exit(1);
    }
    
    printf("\n=== TEST COMPLETED ===\n");
    printf("WiringPi is working correctly!\n");
    printf("\nSuggested connections for complete testing:\n");
    printf("- Connect an LED to GPIO 17 (pin 11) with resistor\n");
    printf("- Connect a button from GPIO 27 (pin 13) to GND\n");
    
    return 0;
}
