#define GRID_SIZE 4
#define SD_ChipSelectPin 2
#define SPEAKER_PIN 9
#define AMP_SHUTDOWN_PIN 8

#include <SdFat.h>
#include <TMRpcm.h>
#include <SPI.h>

enum keys {
  KEY_1 , KEY_2 , KEY_3 , KEY_4 ,
  KEY_5 , KEY_6 , KEY_7 , KEY_8 ,
  KEY_9 , KEY_10, KEY_11, KEY_12,
  KEY_13, KEY_14, KEY_15, KEY_16,
  KEY_NONE
};

enum leds {
  LED_1 , LED_2 , LED_3 , LED_4 ,
  LED_5 , LED_6 , LED_7 , LED_8 ,
  LED_9 , LED_10, LED_11, LED_12,
  LED_13, LED_14, LED_15, LED_16,
  LED_NONE
};

enum sounds {
  SOUND_1 , SOUND_2 , SOUND_3 , SOUND_4 ,
  SOUND_5 , SOUND_6 , SOUND_7 , SOUND_8 ,
  SOUND_9 , SOUND_10, SOUND_11, SOUND_12,
  SOUND_13, SOUND_14, SOUND_15, SOUND_16,
  SOUND_COUNT
};

static const char *sound_files[SOUND_COUNT] = {
  "pluck midi 45.wav", "pluck midi 47.wav", "pluck midi 48.wav", "pluck midi 50.wav",
  "pluck midi 52.wav", "pluck midi 53.wav", "pluck midi 55.wav", "pluck midi 57.wav",
  "pluck midi 59.wav", "pluck midi 60.wav", "pluck midi 62.wav", "pluck midi 64.wav",
  "pluck midi 65.wav", "pluck midi 67.wav", "pluck midi 69.wav", "pluck midi 71.wav"
};

static const uint8_t ROW_PIN[GRID_SIZE] = {A0, A1, A2, A3};
static const uint8_t COL_PIN[GRID_SIZE] = {4, 5, 6, 7};

SdFat sd;
TMRpcm audio;

void setup_keys()
{
  for (size_t i=0; i<GRID_SIZE; i++)
  {
    pinMode(ROW_PIN[i], INPUT_PULLUP);
    pinMode(COL_PIN[i], INPUT);
  }
}

/*
 * Scan a key matrix
 * Return the number of a single pressed key or KEY_NONE
 */
enum keys get_key()
{
  setup_keys();

  enum keys key = KEY_NONE;

  for (size_t x=0; x<GRID_SIZE; x++)
  {
    pinMode     (COL_PIN[x], OUTPUT);
    digitalWrite(COL_PIN[x], LOW);

    for (size_t y=0; y<GRID_SIZE; y++)
    {
      if (digitalRead(ROW_PIN[y]) == LOW)
      {
        key = (y*GRID_SIZE+x);
      }
    }

    pinMode(COL_PIN[x], INPUT);
  }

  return key;
}

void set_led(enum leds led)
{
  uint8_t x = led % GRID_SIZE;
  uint8_t y = led / GRID_SIZE;

  pinMode     (COL_PIN[x], OUTPUT);
  digitalWrite(COL_PIN[x], HIGH);
  pinMode     (ROW_PIN[y], OUTPUT);
  digitalWrite(ROW_PIN[y], LOW);
}

void setup()
{
  // Debug
  Serial.begin(115200);

  // Enable amplifier IC
  pinMode     (AMP_SHUTDOWN_PIN, OUTPUT);
  digitalWrite(AMP_SHUTDOWN_PIN, LOW);

  audio.speakerPin = SPEAKER_PIN;

  // Enable SD-card
  bool sd_ok = sd.begin(SD_ChipSelectPin, SPI_FULL_SPEED);
  Serial.println(sd_ok ? "SD OK" : "SD FAIL");
}

void loop()
{
  static char *sound_file = "";
  static uint8_t active_led = LED_NONE;

  const uint8_t active_key = get_key();

  if (active_key != KEY_NONE)
  {
    if (active_key != active_led)
    {
      active_led = active_key;
      sound_file = sound_files[active_key];
      audio.play(sound_file);

      // Debug
      Serial.print("Playing: ");
      Serial.println(sound_file);
    }
  }

  // Restore led output after using the grid for key input
  set_led(active_led);

  // Amplifier shutdown control
  digitalWrite(AMP_SHUTDOWN_PIN, !audio.isPlaying());

  // Keep the led on for a while to be visible
  delay(1);
}
