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

PGM_P soundFileFromIrCode(uint16_t irCode) {
  switch (irCode) {
    case 0x0140 : return sound_files[1];
    case 0x0158 : return sound_files[2];
    case 0x01A0 : return sound_files[3];
    case 0x0160 : return sound_files[4];
    case 0x0120 : return sound_files[5];
    case 0x0118 : return sound_files[6];
    case 0x01D2 : return sound_files[7];
    case 0x0152 : return sound_files[8];
    default     : return sound_files[16];
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
  const IrCode_t irCode = irDecoder();
  if (irCode.data) {
    audio.play(readString(soundFileFromIrCode(irCode.data)));
  }
}
