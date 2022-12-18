#define IR_QUEUE_SIZE 10
#define SD_ChipSelectPin 2
#define SPEAKER_PIN 9
#define AMP_SHUTDOWN_PIN 8

#include "LG-Audio.h"
#include "LG-ledBtnGrid.h"
#include <cppQueue.h>

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

    /* Not enough processing power for IR, Audio and UART
    Serial.print(irEvent.microsLow);
    Serial.print(" us Low, ");
    Serial.print(irEvent.microsHigh);
    Serial.print(" us High, irState: ");
    Serial.print(irState);
    Serial.println();
    */

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

PGM_P soundFileFromIrCode(unsigned long irCode) {
  switch (irCode) {
    case 0x01FE40BF : return sound_files[1];
    case 0x01FE58A7 : return sound_files[2];
    case 0x01FEA05F : return sound_files[3];
    case 0x01FE609F : return sound_files[4];
    case 0x01FE20DF : return sound_files[5];
    case 0x01FE18E7 : return sound_files[6];
    case 0x01FED22D : return sound_files[7];
    case 0x01FE52AD : return sound_files[8];
    default         : return sound_files[16];
  }
}

void loop() {
  static uint8_t active_led = LED_NONE;
  static uint8_t previous_key = LED_NONE;
  static uint8_t sound_bank_counter = 0;

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

    if (sound_bank_counter < 2) {
      audio.play(readString(sound_files[sound_bank_counter * GRID_SIZE * GRID_SIZE + active_key]));
    } else {
      char numberedFileName[] = "00-00.wav";
      snprintf(numberedFileName, sizeof numberedFileName, "%02d-%02d.wav", sound_bank_counter, active_key);
      audio.play(numberedFileName);
    }

    previous_key = active_key;
  }

  // Amplifier shutdown control
  digitalWrite(AMP_SHUTDOWN_PIN, !audio.isPlaying());

  // Keep the led on for a while to be visible
  leds_refresh();

  // Debug infrared events
  const unsigned long int irCode = irDecoder();
  if (irCode) {
    audio.play(readString(soundFileFromIrCode(irCode)));
    /* Not enough processing power for IR, Audio and UART
    Serial.print("IR Code 0x");
    Serial.print(irCode, HEX);
    Serial.println();
    */
  }
}
