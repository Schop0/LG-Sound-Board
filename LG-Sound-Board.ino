#define GRID_SIZE 4
#define SD_ChipSelectPin 2
#define SPEAKER_PIN 9
#define AMP_SHUTDOWN_PIN 8

#include "pcmConfig.h"

#include <SimpleKeypad.h>
#include <SdFat.h>
#include <TMRpcm.h>

#define KEY_NONE 0
#define LED_NONE 0

static const char *sound_files[] = {
  "",
  "pluck midi 45.wav", "pluck midi 47.wav", "pluck midi 48.wav", "pluck midi 50.wav",
  "pluck midi 52.wav", "pluck midi 53.wav", "pluck midi 55.wav", "pluck midi 57.wav",
  "pluck midi 59.wav", "pluck midi 60.wav", "pluck midi 62.wav", "pluck midi 64.wav",
  "pluck midi 65.wav", "pluck midi 67.wav", "pluck midi 69.wav", "pluck midi 71.wav"
};

static const uint8_t ROW_PIN[GRID_SIZE] = { A0, A1, A2, A3 };
static const uint8_t COL_PIN[GRID_SIZE] = { 4, 5, 6, 7 };
static const char key_chars[GRID_SIZE][GRID_SIZE] = {
  { 1, 2, 3, 4 },
  { 5, 6, 7, 8 },
  { 9, 10, 11, 12 },
  { 13, 14, 15, 16 }
};

static bool pixels[GRID_SIZE][GRID_SIZE];
SimpleKeypad keypad((char *)key_chars, ROW_PIN, COL_PIN, GRID_SIZE, GRID_SIZE);
SdFat sd;
TMRpcm audio;

void setup_grid() {
  for (size_t i = 0; i < GRID_SIZE; i++) {
    pinMode(ROW_PIN[i], INPUT_PULLUP);
    digitalWrite(COL_PIN[i], HIGH);
    pinMode(COL_PIN[i], INPUT);
  }
}

/*
 * Scan a key matrix
 * Return the number of a single pressed key or KEY_NONE
 */
uint8_t get_key() {
  setup_grid();
  return keypad.getKey();
}

void set_led(uint8_t led) {
  led--;
  uint8_t x = led % GRID_SIZE;
  uint8_t y = led / GRID_SIZE;

  // Toggle pixel
  pixels[x][y] = !pixels[x][y];
}

uint8_t IRDebugPin = 0;
ISR(ANALOG_COMP_vect)
{
  if (IRDebugPin)
  {
    digitalWrite(IRDebugPin, bitRead(ACSR, ACO));
  }
}

void test_infrared_receiver(uint8_t outputPin)
{
  bitClear(PRR, PRADC); // Disable ADC power saving
  bitClear(ADCSRA, ADEN); // Disable ADC to use multiplexer for comparator
  bitSet(ADCSRB, ACME); // Analogue Comparator Multiplexer Enable
  bitSet(ACSR, ACBG);  // Select Analog Comparator Bandgap reference voltage
  ADMUX = 6;  // Select ADC6 (pin A6) on the multiplexer
  bitSet(ACSR, ACIE); // Enable interrupts (default on toggle / both edges)
  pinMode(outputPin, OUTPUT); // Prepare output pin
  IRDebugPin = outputPin; // Pass outputPin on to interrupt handler
}

void setup() {
  // Enable amplifier IC
  pinMode(AMP_SHUTDOWN_PIN, OUTPUT);
  digitalWrite(AMP_SHUTDOWN_PIN, LOW);

  audio.speakerPin = SPEAKER_PIN;

  // Enable SD-card
  if (!sd.begin(SD_ChipSelectPin, SPI_FULL_SPEED)) {
    SPCR = 0x00;  // Disable SPI to free up the builtin led
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

  // Point any remote control at the sensor and watch it's signal on the Arduino led
    test_infrared_receiver(LED_BUILTIN);
  }
}

void leds_refresh() {
  static size_t row = 0;

  // Disable all leds
  setup_grid();

  // Write pixels to columns
  for (size_t col = 0; col < GRID_SIZE; col++) {
    digitalWrite(COL_PIN[col], pixels[col][row]);
    pinMode(COL_PIN[col], OUTPUT);
  }

  // Activate row for a short time, blocking
  pinMode(ROW_PIN[row], OUTPUT);
  digitalWrite(ROW_PIN[row], LOW);
  delay(1);
  pinMode(ROW_PIN[row], INPUT_PULLUP);

  // Prepare the next row number
  ++row %= GRID_SIZE;
}

void loop() {
  static uint8_t active_led = LED_NONE;

  const uint8_t active_key = get_key();

  if (active_key != KEY_NONE) {
    // Remember led after key deactivates
    set_led(active_key);

    // Play a sound in the background (non-blocking)
    audio.play(sound_files[active_key]);
  }

  // Amplifier shutdown control
  digitalWrite(AMP_SHUTDOWN_PIN, !audio.isPlaying());

  // Keep the led on for a while to be visible
  leds_refresh();
}
