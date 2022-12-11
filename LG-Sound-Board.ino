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
#define SOUND_BANK 0


static uint8_t sound_bank_counter = 0;
static const uint8_t NUMBER_OF_SOUND_BANKS = 2;

const char filename_0[] PROGMEM = "";
const char filename_1[] PROGMEM = "";
const char filename_2[] PROGMEM = "ambulance.wav";
const char filename_3[] PROGMEM = "mobile.wav";
const char filename_4[] PROGMEM = "horn.wav";
const char filename_5[] PROGMEM = "handgun.wav";
const char filename_6[] PROGMEM = "transition.wav";
const char filename_7[] PROGMEM = "windows_95.wav";
const char filename_8[] PROGMEM = "movement.wav";
const char filename_9[] PROGMEM = "train_whistle.wav";
const char filename_10[] PROGMEM = "ghost_whoosh.wav";
const char filename_11[] PROGMEM = "window_break.wav";
const char filename_12[] PROGMEM = "matrix_printer.wav";
const char filename_13[] PROGMEM = "bullet.wav";
const char filename_14[] PROGMEM = "dog.wav";
const char filename_15[] PROGMEM = "dial_up.wav";
const char filename_16[] PROGMEM = "futuristic_gunshot.wav";
const char filename_17[] PROGMEM = "";
const char filename_18[] PROGMEM = "glass.wav";
const char filename_19[] PROGMEM = "glass.wav";
const char filename_20[] PROGMEM = "glass.wav";
const char filename_21[] PROGMEM = "glass.wav";
const char filename_22[] PROGMEM = "glass.wav";
const char filename_23[] PROGMEM = "glass.wav";
const char filename_24[] PROGMEM = "glass.wav";
const char filename_25[] PROGMEM = "glass.wav";
const char filename_26[] PROGMEM = "glass.wav";
const char filename_27[] PROGMEM = "glass.wav";
const char filename_28[] PROGMEM = "glass.wav";
const char filename_29[] PROGMEM = "glass.wav";
const char filename_30[] PROGMEM = "glass.wav";
const char filename_31[] PROGMEM = "glass.wav";
const char filename_32[] PROGMEM = "glass.wav";

PGM_P sound_files[] =
    {
        filename_0,
        filename_1,
        filename_2,
        filename_3,
        filename_4,
        filename_5,
        filename_6,
        filename_7,
        filename_8,
        filename_9,
        filename_10,
        filename_11,
        filename_12,
        filename_13,
        filename_14,
        filename_15,
        filename_16,
        filename_17,
        filename_18,
        filename_19,
        filename_20,
        filename_21,
        filename_22,
        filename_23,
        filename_24,
        filename_25,
        filename_26,
        filename_27,
        filename_28,
        filename_29,
        filename_30,
        filename_31,
        filename_32};

static const uint8_t ROW_PIN[GRID_SIZE] = {A0, A1, A2, A3};
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

void setup_grid(uint8_t rowMode) {
  for (size_t i = 0; i < GRID_SIZE; i++) {
    pinMode(ROW_PIN[i], rowMode);
    digitalWrite(COL_PIN[i], HIGH);
    pinMode(COL_PIN[i], INPUT);
  }
}

/*
 * Scan a key matrix
 * Return the number of a single pressed key or KEY_NONE
 */
uint8_t get_key() {
  setup_grid(INPUT_PULLUP);
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

char *readString(PGM_P flash_ptr)
{
  static char stringbuf[32] = {'\0'};
  strcpy_P(stringbuf, flash_ptr);
  return stringbuf;
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
  setup_grid(INPUT);

  // Write pixels to columns
  for (size_t col = 0; col < GRID_SIZE; col++) {
    if (pixels[col][row])
    {
      digitalWrite(COL_PIN[col], HIGH);
      pinMode(COL_PIN[col], OUTPUT);
    }
  }

  // Activate row for a short time, blocking
  pinMode(ROW_PIN[row], OUTPUT);
  digitalWrite(ROW_PIN[row], LOW);
  delay(1);
  pinMode(ROW_PIN[row], INPUT);

  // Prepare the next row number
  ++row %= GRID_SIZE;
}

void loop() {
  static uint8_t active_led = LED_NONE;
  static uint8_t previous_key = LED_NONE;

  uint8_t active_key = get_key();

  if (active_key == KEY_NONE) {
    // Geen key, doe huidige led uit als klaar met audio
    if (!audio.isPlaying()) {
      if (active_led != LED_NONE) {
        set_led(active_led);
        active_led = LED_NONE;
      }
    }
  } else if (active_key == 1) {
    // switch mode
    // press first button for changing sound bank
    sound_bank_counter++;
    sound_bank_counter %= NUMBER_OF_SOUND_BANKS;
    active_key = KEY_NONE;
  } else if (previous_key == active_key) {
    set_led(active_led);
    active_led = LED_NONE;
    active_key = KEY_NONE;
    previous_key = KEY_NONE;
    audio.stopPlayback();
  } else {
    if (previous_key != KEY_NONE) {
      set_led(active_led);
    }

    set_led(active_key);
    active_led = active_key;

    audio.play(readString(sound_files[sound_bank_counter * GRID_SIZE * GRID_SIZE + active_key]));

    previous_key = active_key;
  }

  // Amplifier shutdown control
  digitalWrite(AMP_SHUTDOWN_PIN, !audio.isPlaying());

  // Keep the led on for a while to be visible
  leds_refresh();
}
