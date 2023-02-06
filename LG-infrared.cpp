#include "Arduino.h"
#include "LG-infrared.h"
#include <cppQueue.h>

typedef struct {
  bool pinState;
  unsigned long timeStamp_micros;
} irEvent_t;

irEvent_t irEventQueueData[IR_QUEUE_SIZE];

typedef enum {
  IDLE,
  START_MARK,
  START_SPACE,
  BIT_MARK,
  BIT_SPACE,
  STOP
} necProtocolState_t;

typedef struct {
  uint32_t data;
  size_t bitCount;
  necProtocolState_t txState;
} laserMessage_t;

static laserMessage_t laserMessage = {0};

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
  const irEvent_t event = {
    .pinState         = bitRead(ACSR, ACO),
    .timeStamp_micros = micros()
  };

  irEventQueue.push(&event);
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

void irInit(uint16_t playerId) {
  irInitReceiver();
  irInitTransmitter();
}

/*
 * Decode NEC-style raw received bits into valid data or zero
 */
IrCode_t decodeNEC(uint32_t rawData) {
  // The NEC protocol consists of 8-bit address and 8-bit data.
  // Each followed by their logical inverse for a total of 4 bytes.
  const uint8_t  address = (rawData >>  0) & 0xff;
  const uint8_t iaddress = (rawData >>  8) & 0xff;
  const uint8_t  command = (rawData >> 16) & 0xff;
  const uint8_t icommand = (rawData >> 24) & 0xff;

  IrCode_t decoded = {0};

  // Verify the address and command match their inverse versions.
  if ( ((uint8_t)~address == iaddress) && ((uint8_t)~command == icommand) ) {
    decoded.address = address;
    decoded.command = command;
  }

  return decoded;
}

/*
 * Encode NEC-style data into raw bits ready to transmit
 */
uint32_t encodeNEC(IrCode_t decoded) {
  uint32_t rawData = 0;

  rawData |=   decoded.address  <<  0;
  rawData |= ~(decoded.address) <<  8;
  rawData |=   decoded.command  << 16;
  rawData |= ~(decoded.command) << 24;

  return rawData;
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
  static unsigned long timePrevious_micros = 0;

  unsigned long timeDelta_micros = 0;
  IrCode_t returnData = {0};
  irEvent_t irEvent;

  while (!irEventQueue.isEmpty()) {
    // Temporarily disable interrupts for an atomic queue operation
    // May interfere with time critical audio
    noInterrupts();
    irEventQueue.pop(&irEvent);
    interrupts();

    // Protocol specific decoding with magic numbers for timing
    timeDelta_micros    = irEvent.timeStamp_micros - timePrevious_micros;
    timePrevious_micros = irEvent.timeStamp_micros;

    // Decode based on time between pulses
    if (irEvent.pinState == 1) {
      if (timeDelta_micros > 2000) {
        // Start bit: 4500us
        irData = 0;
        irBit  = 0;
      } else {
        // Data bit
        if (timeDelta_micros > 1000) {
          // Logic one: 1687.5us
          bitSet(irData, irBit);
        } else {
          // Logic zero: 562.5us
          bitClear(irData, irBit);
        }
        irBit++;
      }
      // Assume data ends after 32 bits
      if (irBit >= 32) {
        returnData = decodeNEC(irData);
        irBit  = 0;
      }
    }
  }

  return returnData;
}

bool fireLaser(unsigned int playerNumber) {
  // LG-protocol: 8-bit codes starting with 0b1010 and a 4-bit plyer number
  // Skip player numbers that may be misinterpreted ad the start bits
  static const char player[] = {0x00, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, /*skip*/ 0xA6, 0xA7, 0xA8, 0xA9, /*skip*/ 0xAB, 0xAC, /*skip*/ 0xAE, 0xAF};
  static const unsigned int playerCount = sizeof(player) / sizeof(player[0]);

  if (playerNumber >= playerCount) {
    return false;
  } else
  if (laserMessage.txState != IDLE) {
    return false;
  } else {
    laserMessage.data     = encodeNEC({player[playerNumber]});
    laserMessage.bitCount = 8 * sizeof(laserMessage.data);
    laserMessage.txState  = START_MARK;
    irLedOff();
    return true;
  }
}

void runLasergame()
{
  static unsigned long nextEventTime = 0;
  static size_t bitNumber = 0;

  const unsigned long currentTime = micros();

  // Return immediately until next event is due
  if (nextEventTime > currentTime) {
    return;
  }

  unsigned long eventDuration = 0;

  switch (laserMessage.txState) {
    case START_MARK  : irLedOn (); eventDuration = 9000; laserMessage.txState = START_SPACE ; bitNumber = 0; break;
    case START_SPACE : irLedOff(); eventDuration = 4500; laserMessage.txState = BIT_MARK    ; break;
    case BIT_MARK    : irLedOn (); eventDuration =  562; laserMessage.txState = BIT_SPACE   ; break;
    case BIT_SPACE   : irLedOff(); eventDuration = bitRead(laserMessage.data, bitNumber++)
        ? 1687 // Logic one
        : 562; // Logic zero
      laserMessage.txState = (bitNumber < laserMessage.bitCount)
        ? BIT_MARK
        : STOP;
      break;
    case STOP        : irLedOn (); eventDuration =  562; laserMessage.txState = IDLE        ; break;
    default          : irLedOff(); eventDuration =    0; laserMessage.txState = IDLE        ; break;
  }

  nextEventTime = currentTime + eventDuration;

  // Debug IR led on IO pin
  digitalWrite(PIN_A4, irLedState());
}
