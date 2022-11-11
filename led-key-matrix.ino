#define GRID_SIZE 4

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

static const uint8_t ROW_PIN[GRID_SIZE] = {A0, A1, A2, A3};
static const uint8_t COL_PIN[GRID_SIZE] = {4, 5, 6, 7};

/*
 *
 */
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

  if ((LED_NONE == led) || (y >= GRID_SIZE))
  {
    return;
  }
  else
  {
    pinMode     (COL_PIN[x], OUTPUT);
    digitalWrite(COL_PIN[x], HIGH);
    pinMode     (ROW_PIN[y], OUTPUT);
    digitalWrite(ROW_PIN[y], LOW);
  }
}

void setup()
{
  // Debug
  Serial.begin(115200);
  
  // Enable amplifier IC using pin 8
  pinMode     (8, OUTPUT);
  digitalWrite(8, LOW);
}

void loop()
{
  static uint8_t volume = 1; // 0-127 for analogWrite()
  static uint8_t active_led = LED_NONE;

  const uint8_t active_key = get_key();

  if (active_key != KEY_NONE)
  {
    // Key beep on speaker
    analogWrite(9, volume);

    if (active_key != active_led)
    {
      active_led = active_key;

      // Debug led change events
      Serial.print("Led: ");
      Serial.println(active_led+1);
    }
  }
  else
  {
    // Stop beeping
    analogWrite(9, 0);
  }

  // Restore led output after using the grid for key input
  set_led(active_led);

  // Keep the led on for a while to be visible
  delay(1);
}
