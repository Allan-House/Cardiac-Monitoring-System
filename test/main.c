#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#define LED_PIN 17      // GPIO 17 (physical pin 11)
#define BUTTON_PIN 27   // GPIO 27 (physical pin 13)

#define ADS1115_CONVERSION_REG 0x00
#define ADS1115_CONFIG_REG     0x01

// (Big Endian <-> Little Endian)
static inline uint16_t swap16(uint16_t x) { 
    return ((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8); 
}

int test_gpio() {
    printf("\n=== GPIO TEST ===\n");
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pullUpDnControl(BUTTON_PIN, PUD_UP);
    printf("Pins configured successfully\n");

    printf("Testing LED blink (3 cycles)...\n");
    for(int i = 0; i < 3; i++) {
        printf("LED ON (%d/3)\n", i+1);
        digitalWrite(LED_PIN, HIGH);
        usleep(300000); // 300ms
        
        printf("LED OFF\n");
        digitalWrite(LED_PIN, LOW);
        usleep(300000);
    }
    
    printf("Testing button (3 seconds)...\n");
    printf("Button states: ");
    fflush(stdout);
    
    for(int i = 0; i < 30; i++) {
        int button_state = digitalRead(BUTTON_PIN);
        printf("%d", button_state);
        fflush(stdout);
        usleep(100000); // 100ms
    }
    printf("\n");

    digitalWrite(LED_PIN, LOW);
    printf("GPIO test completed!\n");
    return 0;
}

int test_i2c_basic(int address) {
    printf("Testing basic I2C communication at 0x%02X... ", address);
    fflush(stdout);
    
    int fd = wiringPiI2CSetup(address);
    if (fd < 0) {
        printf("FAILED (setup error: %s)\n", strerror(errno));
        return 0;
    }
    
    int result = wiringPiI2CRead(fd);
    if (result < 0) {
        printf("FAILED (read error: %s)\n", strerror(errno));
        close(fd);
        return 0;
    }
    
    printf("OK (read byte: 0x%02X)\n", result);
    close(fd);
    return 1;
}

int test_known_devices() {
    printf("\n=== TESTING KNOWN I2C DEVICES ===\n");
    
    struct {
        int addr;
        const char* name;
    } known_devices[] = {
        {0x48, "ADS1115 (ADDR=GND)"},
        {0x49, "ADS1115 (ADDR=VDD)"},
        {0x4A, "ADS1115 (ADDR=SDA)"},
        {0x4B, "ADS1115 (ADDR=SCL)"},
    };
    
    int found_count = 0;
    for (int i = 0; i < sizeof(known_devices)/sizeof(known_devices[0]); i++) {
        printf("Testing %s at 0x%02X... ", known_devices[i].name, known_devices[i].addr);
        if (test_i2c_basic(known_devices[i].addr)) {
            found_count++;
        }
    }
    
    printf("\nTotal responsive devices found: %d\n", found_count);
    return found_count;
}

int main() {
    printf("=== WIRINGPI I2C DEBUG TEST ===\n");
    
    if (getuid() != 0) {
        printf("WARNING: Not running as root. May need sudo for I2C access.\n\n");
    }
    
    printf("Initializing WiringPi...\n");
    if(wiringPiSetupGpio() == -1) {
        fprintf(stderr, "ERROR: Failed to initialize WiringPi!\n");
        exit(1);
    }
    printf("WiringPi initialized\n");
    
    if(test_gpio() != 0) {
        fprintf(stderr, "ERROR: GPIO test failed!\n");
        exit(1);
    }
    
    test_known_devices();
   
    return 0;
}
