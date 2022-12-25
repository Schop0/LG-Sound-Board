#define SD_ChipSelectPin 2
#define SPEAKER_PIN 9
#define AMP_SHUTDOWN_PIN 8

#include "LG-Audio.h"
#include "LG-ledBtnGrid.h"
#include "LG-infrared.h"

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
}

const char *soundFileFromKey(unsigned int bank, unsigned int key) {
  static char fileName[50] = "";
  const unsigned int lineNumber = bank * NUMBER_OF_SOUNDS_PER_BANK + key;

  // The SD-card cannot simply be used by audio and main simultaneously
  audio.stopPlayback();

  SdFile index("index.txt", O_READ);

  for (uint8_t line=0; line < lineNumber; line++) {
    index.fgets(fileName, sizeof fileName);
  }

  fileName[strcspn(fileName, "\r\n")] = '\0';

  return fileName;
}

const char *soundFileFromIrCode(uint16_t irCode) {
  switch (irCode) {
    case 0x0140 : return soundFileFromKey(0, 1);
    case 0x0158 : return soundFileFromKey(0, 2);
    case 0x01A0 : return soundFileFromKey(0, 3);
    case 0x0160 : return soundFileFromKey(0, 4);
    case 0x0120 : return soundFileFromKey(0, 5);
    case 0x0118 : return soundFileFromKey(0, 6);
    case 0x01D2 : return soundFileFromKey(0, 7);
    case 0x0152 : return soundFileFromKey(0, 8);
    default     : return soundFileFromKey(0, 16);
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
  }

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
