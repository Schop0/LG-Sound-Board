#define GRID_SIZE 4
#define IR_QUEUE_SIZE 10

#define SD_ChipSelectPin 2
#define SPEAKER_PIN 9
#define AMP_SHUTDOWN_PIN 8

#include "pcmConfig.h"

#include <SimpleKeypad.h>
#include <SdFat.h>
#include <TMRpcm.h>
#include <cppQueue.h>

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

typedef struct {
  uint16_t microsLow;
  uint16_t microsHigh;
} irEvent_t;

irEvent_t irEventQueueData[IR_QUEUE_SIZE];

cppQueue irEventQueue(
  sizeof(irEvent_t), // Size of record
  IR_QUEUE_SIZE, // Number of records
  FIFO, // FIFO or LIFO operation
  false, // Overwrite oldest when full
  irEventQueueData, // Static memory pointer
  sizeof(irEventQueueData) // Static memory size
);

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

ISR(ANALOG_COMP_vect)
{
  static unsigned long timeLast = 0;
  static irEvent_t event = { 0 };

  const unsigned long timeNow = micros();
  unsigned long timeDelta = timeNow - timeLast;
  timeLast = timeNow;

  // Timeout on overflow
  if (timeDelta > UINT16_MAX)
  {
    timeDelta = 0;
  }

  // Capture low and high timings, then fire an event
  switch (bitRead(ACSR, ACO)) {
    case HIGH:
      event.microsLow = timeDelta;
      break;
    case LOW:
      event.microsHigh = timeDelta;
      irEventQueue.push(&event);
      event.microsHigh = 0;
      event.microsLow = 0;
      break;
  }
}

void test_infrared_receiver()
{
  bitClear(PRR, PRADC); // Disable ADC power saving
  bitClear(ADCSRA, ADEN); // Disable ADC to use multiplexer for comparator
  bitSet(ADCSRB, ACME); // Analogue Comparator Multiplexer Enable
  bitSet(ACSR, ACBG);  // Select Analog Comparator Bandgap reference voltage
  ADMUX = 6;  // Select ADC6 (pin A6) on the multiplexer
  bitSet(ACSR, ACIE); // Enable interrupts (default on toggle / both edges)
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
  }

  Serial.begin(115200);
  test_infrared_receiver();
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

/*
 * Infrared decoder pseudo-statemachine
 * Protocol is reverse engineered from an arbitrary protocol. In this case:
 * the remote control of the KEF LS50 Wireless speakers
 * This remote transmits about 32 bits of data and some headers
 * Different high and low durations translate to different bits or header fields
 */
unsigned long int irDecoder() {
  static unsigned long int irData = 0;
  static unsigned int irBit = 0;

  unsigned long int returnData = 0;
  irEvent_t irEvent;
  enum {
    SHORT = 0,
    LONG = 1,
    START = 2,
    END = 3,
  } irState;

  while (!irEventQueue.isEmpty()) {
    // Temporarily disable interrupts for an atomic queue operation
    // May interfere with time critical audio
    noInterrupts();
    irEventQueue.pop(&irEvent);
    interrupts();

    // Protocol specific decoding with magic numbers for timing
    if (irEvent.microsLow == 0) {
      irState = START;
    } else if (irEvent.microsHigh > 1000) {
      irState = END;
    } else if (irEvent.microsLow > 2000) {
      irState = START;
    } else {
      irState = (irEvent.microsLow < 1000)
              ? SHORT
              : LONG;
    }

    Serial.print(irEvent.microsLow);
    Serial.print(" us Low, ");
    Serial.print(irEvent.microsHigh);
    Serial.print(" us High, irState: ");
    Serial.print(irState);
    Serial.println();

    switch (irState) {
      case START :
        irData = 0;
        break;
      case SHORT :
        irBit++;
        // Shift in a zero
        irData <<= 1;
        break;
      case LONG :
        irBit++;
        // Shift in a one
        irData <<= 1;
        irData |= 1;
        break;
      case END :
        returnData = irData;
        break;
    }
  }

  return returnData;
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

  // Debug infrared events
  const unsigned long int irCode = irDecoder();
  if (irCode) {
    Serial.print("IR Code 0x");
    Serial.print(irCode, HEX);
    Serial.println();
  }
}
