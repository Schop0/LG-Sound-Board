#include "Arduino.h"
#include "LG-ledBtnGrid.h"
#include <SimpleKeypad.h>

#define KEY_DEBOUNCE_MS (10U)

#define ROW_MASK (0x0fU)
#define COL_MASK (0xf0U)

// Matrix rows: Port C bits 0-3
static const uint8_t ROW_PIN[GRID_SIZE] = {A0, A1, A2, A3};
// Matrix columns: Port D bits 4-7
static const uint8_t COL_PIN[GRID_SIZE] = { 4, 5, 6, 7 };
static const char key_chars[GRID_SIZE][GRID_SIZE] = {
  { 1, 2, 3, 4 },
  { 5, 6, 7, 8 },
  { 9, 10, 11, 12 },
  { 13, 14, 15, 16 }
};

static bool keyScanTrigger = true;
static bool pixels[GRID_SIZE][GRID_SIZE];
SimpleKeypad keypad((char *)key_chars, ROW_PIN, COL_PIN, GRID_SIZE, GRID_SIZE);

void setup_grid(uint8_t rowMode) {
  uint8_t oldSREG = SREG; // Save interrupt state
  cli();                  // Disable interrupts

  DDRC &= ~ROW_MASK;      // Rows input
  if (rowMode == INPUT) {
		PORTC &= ~ROW_MASK;   // Rows tri-stated
  } else {
    PORTC |= ROW_MASK;    // Rows pullup
  }
  PORTD |=  COL_MASK;     // Columns high/pullup
  DDRD  &= ~COL_MASK;     // Columns input
  PORTD &= ~COL_MASK;     // Columns tri-state

  SREG = oldSREG;         // Restore interrupt state

  return;
}

/*
 * Scan a key matrix
 * Return the number of a single pressed key or KEY_NONE
 */
uint8_t get_key() {
  if (!keyScanTrigger) {
    return KEY_NONE;
  }
  keyScanTrigger = false;

  setup_grid(INPUT_PULLUP);
  uint8_t key = keypad.scan();

  static bool hold = false;
  if (key && !hold) {
    hold = true;
    return key;
  } else
  if (!key) {
    hold = false;
  }

  return KEY_NONE;
}

void set_led(uint8_t led) {
  led--;
  uint8_t x = led % GRID_SIZE;
  uint8_t y = led / GRID_SIZE;

  // Toggle pixel
  pixels[x][y] = !pixels[x][y];
}

void leds_refresh() {
  static size_t row = 0;
  static int lastRefresh = 0;

  // Disable all leds
  setup_grid(INPUT);

  uint8_t oldSREG = SREG; // Save interrupt state
  cli();                  // Disable interrupts

  // Write pixels to columns
  for (size_t col = 0; col < GRID_SIZE; col++) {
    if (pixels[col][row])
    {
      uint8_t bit = 0x10U << col;
      PORTD |= bit; // High
      DDRD  |= bit; // Output
    }
  }

  // Activate row
  uint8_t bit = 0x01U << row;
  DDRC  |=  bit; // Output
  PORTC &= ~bit; // Low

  // Restore interrupt state
  SREG = oldSREG;

  // Prepare the next row number
  const int timeNow = millis();
  if (timeNow > lastRefresh) {
    lastRefresh = timeNow;
    ++row %= GRID_SIZE;

    // Synchronise keypad scanning with led changeover
    if ((timeNow % KEY_DEBOUNCE_MS) == 0) {
      keyScanTrigger = true;
    }
  }
}
