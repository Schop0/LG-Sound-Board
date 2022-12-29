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

void runLasergame()
{
  digitalWrite(PIN_A4, !digitalRead(PIN_A4));
  if(digitalRead(PIN_A4))
  {
    irLedOn();
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    irLedOff();
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void fire() {
  unsigned char personID = 0b10100010;
  int bitnr = 0;

  while (bitnr < 8)
  {
    if (personID & 0x01)
    {
      // 1
      sendBit(1);
    }
    else
    {
      // 0
      sendBit(0);
    }

    bitnr++;
    personID >>= 1;
  }
}

void sendBit(uint8_t bit){
  if ((time + 1000) <= millis())
  {
    if (bit==0) {
      digitalWrite(PIN_A4, LOW);
    } else {
      digitalWrite(PIN_A4, HIGH);
    }
    
    time = millis();
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
  } else if (active_key == 2) {
    fire();
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
