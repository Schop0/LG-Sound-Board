#include <stdint.h>

#define GRID_SIZE 4
#define KEY_NONE 0
#define LED_NONE 0

// void setup_grid(uint8_t rowMode);
uint8_t get_key();
void set_led(uint8_t led);
void set_led(uint8_t led, uint8_t status);
void leds_refresh();
