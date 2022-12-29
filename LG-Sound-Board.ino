#define SD_ChipSelectPin 2
#define SPEAKER_PIN 9
#define AMP_SHUTDOWN_PIN 8

#include "LG-Audio.h"
#include "LG-ledBtnGrid.h"
#include "LG-infrared.h"

static unsigned int time = 0;

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

  irInit();

  initSoundBankCount();

  pinMode(PIN_A4, OUTPUT);

  time = millis();

  Serial.begin(115200);
}

unsigned int irPlayerCode = 0;
int irBitCount = 0;
bool irBusy = false;

void runLasergame()
{
  int bitTime_ms = 10;
  if ((time + bitTime_ms) <= millis())
  {
    time += bitTime_ms;

    if (irBitCount+1) {
      if(irPlayerCode & (1 << irBitCount)) {
        irLedOn();
        digitalWrite(PIN_A4, HIGH); // Debug
      } else {
        irLedOff();
        digitalWrite(PIN_A4, LOW); // Debug
      }
      irBitCount--;
    } else {
        irLedOff();
        digitalWrite(PIN_A4, LOW); // Debug
    }
  }
}

bool fireLaser(unsigned int playerNumber) {
  // LG-protocol: 8-bit codes starting with 0b1010 and a 4-bit plyer number
  // Skip player numbers that may be misinterpreted ad the start bits
  static const char player[] = {0x00, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, /*skip*/ 0xA6, 0xA7, 0xA8, 0xA9, /*skip*/ 0xAB, 0xAC, /*skip*/ 0xAE, 0xAF};
  static const unsigned int playerCount = sizeof(player) / sizeof(player[0]);
  static const unsigned int playerCodeLength_bits = 8;

  if (playerNumber >= playerCount) {
    return false;
  } else
  if (irBusy) {
    return false;
  } else {
    irPlayerCode = player[playerNumber];
    irBitCount = playerCodeLength_bits;
    return true;
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

    audio.play(soundFileFromKey(sound_bank_counter, active_key));

    previous_key = active_key;

    // Use key number as lasergame player number
    fireLaser(active_key);
  }

  // Run the lasergame background process to handle transmitting
  runLasergame();

  // Amplifier shutdown control
  digitalWrite(AMP_SHUTDOWN_PIN, !audio.isPlaying());

  // Keep the led on for a while to be visible
  leds_refresh();

  // Debug infrared events
  const IrCode_t irCode = irDecoder();
  if (irCode.data) {
    audio.play(soundFileFromIrCode(irCode.data));
  }
}
