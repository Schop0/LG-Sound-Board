#define GRID_SIZE 4
#define SD_ChipSelectPin 2
#define SPEAKER_PIN 9
#define AMP_SHUTDOWN_PIN 8

#include <SimpleKeypad.h>
#include <SdFat.h>
#include <TMRpcm.h>
#include <SPI.h>

#define KEY_NONE 0
#define LED_NONE 0

static const char *sound_files[] = {
  "",
  "pluck midi 45.wav", "pluck midi 47.wav", "pluck midi 48.wav", "pluck midi 50.wav",
  "pluck midi 52.wav", "pluck midi 53.wav", "pluck midi 55.wav", "pluck midi 57.wav",
  "pluck midi 59.wav", "pluck midi 60.wav", "pluck midi 62.wav", "pluck midi 64.wav",
  "pluck midi 65.wav", "pluck midi 67.wav", "pluck midi 69.wav", "pluck midi 71.wav"
};

static const uint8_t ROW_PIN[GRID_SIZE] = {A0, A1, A2, A3};
static const uint8_t COL_PIN[GRID_SIZE] = {4, 5, 6, 7};
static const char key_chars[GRID_SIZE][GRID_SIZE] = {
  { 1,  2,  3,  4},
  { 5,  6,  7,  8},
  { 9, 10, 11, 12},
  {13, 14, 15, 16}
};

SimpleKeypad keypad((char *)key_chars, ROW_PIN, COL_PIN, GRID_SIZE, GRID_SIZE);
SdFat sd;
TMRpcm audio;

void setup_keys()
{
  for (size_t i=0; i<GRID_SIZE; i++)
  {
    pinMode(ROW_PIN[i], INPUT_PULLUP);
    digitalWrite(COL_PIN[i], HIGH);
    pinMode(COL_PIN[i], INPUT);
  }
}

/*
 * Scan a key matrix
 * Return the number of a single pressed key or KEY_NONE
 */
uint8_t get_key()
{
  setup_keys();
  return keypad.getKey();
}

void set_led(uint8_t led)
{
  led--;
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
    active_led = active_key;
    sound_file = sound_files[active_key];
    audio.play(sound_file);

    // Debug
    Serial.print("Playing: ");
    Serial.println(sound_file);
  }

  // Restore led output after using the grid for key input
  set_led(active_led);

  // Amplifier shutdown control
  digitalWrite(AMP_SHUTDOWN_PIN, !audio.isPlaying());

  // Keep the led on for a while to be visible
  delay(1);
}
