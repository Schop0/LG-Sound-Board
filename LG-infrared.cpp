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

void irInit()
{
  bitClear(PRR, PRADC); // Disable ADC power saving
  bitClear(ADCSRA, ADEN); // Disable ADC to use multiplexer for comparator
  bitSet(ADCSRB, ACME); // Analogue Comparator Multiplexer Enable
  bitSet(ACSR, ACBG);  // Select Analog Comparator Bandgap reference voltage
  ADMUX = 6;  // Select ADC6 (pin A6) on the multiplexer
  bitSet(ACSR, ACIE); // Enable interrupts (default on toggle / both edges)
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
