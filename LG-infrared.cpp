#include "LG-infrared.h"
#include <cppQueue.h>
#include <Arduino.h>

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

void irInitReceiver()
{
  bitClear(PRR, PRADC); // Disable ADC power saving
  bitClear(ADCSRA, ADEN); // Disable ADC to use multiplexer for comparator
  bitSet(ADCSRB, ACME); // Analogue Comparator Multiplexer Enable
  bitSet(ACSR, ACBG);  // Select Analog Comparator Bandgap reference voltage
  ADMUX = 6;  // Select ADC6 (pin A6) on the multiplexer
  bitSet(ACSR, ACIE); // Enable interrupts (default on toggle / both edges)
}

void irInitTransmitter() {
  // Turn off pin output driver
  irLedOff();
  // Disable Timer2 power saving
  bitClear(PRR, PRTIM2);
  // Reset Timer2 control registers
  TCCR2A = 0;
  TCCR2B = 0;
  // COM2B1:0: Compare Match Output B Mode
  // Mode 2: Clear OC2B on compare match
  bitSet(TCCR2A, COM2B1);
  // WGM22:0: Waveform Generation Mode
  // Mode 7: Fast PWM counting up to OCR2A
  bitSet(TCCR2A, WGM20);
  bitSet(TCCR2A, WGM21);
  bitSet(TCCR2B, WGM22);
  // CS22:0: Clock Select
  // clk/8 (from prescaler)
  bitSet(TCCR2B, CS21);
  // OCR2A sets frequency (clock frequency / prescaler / output frequency)
  OCR2A = (16000000 / 8 / 38000);
  // OCR2B sets duty cycle (relative to OCR2A)
  OCR2B = OCR2A / 2;
}

void irInit() {
  irInitReceiver();
  irInitTransmitter();
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

    // Debug
    Serial.print("irEvent.microsHigh: "); Serial.println(irEvent.microsHigh);

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
