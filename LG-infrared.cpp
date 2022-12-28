#include "Arduino.h"
#include "LG-infrared.h"
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

// Check timer interrupt interval
unsigned long t=0, t0=0, t1=0;

// Timer0 compare match A interrupt handler
ISR(TIMER0_COMPA) {
  t1 = micros();
  t = t1 - t0;
}

// Initialise timer 0 to generate protocol timing interrupts
void irInitProtocolNEC() {
  // Disable power saving
  bitClear(PRR, PRTIM0);
  bitClear(PRR, PRTIM2);
  // Reset control registers
  TCCR0A = 0;
  TCCR0B = 0;
  // Clock Select
  // clk/1024 (from prescaler)
  bitSet(TCCR0B, CS00);
  bitSet(TCCR0B, CS02);
  // Reset match registers
  OCR0A = 0;
  OCR0B = 0;
  // Reset counter register
  TCNT0 = 0;
  t0 = micros();
  // Clear pending interrupts
  bitSet(TIFR0, OCF0A);
  // Enable match interrupts
  bitSet(TIMSK0, OCIE0A);

  // Test interrupting at match value 157 (10.048ms)
  TCCR0A = 157;
  // Wait for interrupt to have fired
  while (!t);
  // Print result
  Serial.print("t0: ");
  Serial.print(t0);
  Serial.print("us t1: ");
  Serial.print(t1);
  Serial.print("us t delta: ");
  Serial.print(t);
  Serial.print("us");
  Serial.println();
}

void irInit() {
  irInitReceiver();
  irInitTransmitter();
  irInitProtocolNEC();
}

/*
 * Decode NEC-style raw received bits into valid data or zero
 */
IrCode_t decodeNEC(uint32_t rawData) {
  // The NEC protocol consists of 8-bit address and 8-bit data.
  // Each followed by their logical inverse for a total of 4 bytes.
  uint8_t *rawBytes = (uint8_t *) &rawData;
  const uint8_t  address = rawBytes[3];
  const uint8_t iaddress = rawBytes[2];
  const uint8_t  command = rawBytes[1];
  const uint8_t icommand = rawBytes[0];

  IrCode_t decoded = {0};

  // Verify the address and command match their inverse versions.
  if ( ((uint8_t)~address == iaddress) && ((uint8_t)~command == icommand) ) {
    decoded.address = address;
    decoded.command = command;
  }

  return decoded;
}

/*
 * Infrared decoder pseudo-statemachine
 * Protocol is reverse engineered from an arbitrary protocol. In this case:
 * the remote control of the KEF LS50 Wireless speakers
 * This remote seems to transmit NEC-style codes
 * Different high and low durations translate to different bits or header fields
 */
IrCode_t irDecoder() {
  static uint32_t irData = 0;
  static unsigned int irBit = 0;

  IrCode_t returnData = {0};
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
        returnData = decodeNEC(irData);
        break;
    }
  }

  return returnData;
}
