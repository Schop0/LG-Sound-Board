#include "LG-Audio.h"

SdFat sd;
TMRpcm audio;

const char filename_0[] PROGMEM = "";
const char filename_1[] PROGMEM = "";
const char filename_2[] PROGMEM = "ambulance.wav";
const char filename_3[] PROGMEM = "mobile.wav";
const char filename_4[] PROGMEM = "horn.wav";
const char filename_5[] PROGMEM = "handgun.wav";
const char filename_6[] PROGMEM = "transition.wav";
const char filename_7[] PROGMEM = "windows_95.wav";
const char filename_8[] PROGMEM = "movement.wav";
const char filename_9[] PROGMEM = "train_whistle.wav";
const char filename_10[] PROGMEM = "ghost_whoosh.wav";
const char filename_11[] PROGMEM = "window_break.wav";
const char filename_12[] PROGMEM = "matrix_printer.wav";
const char filename_13[] PROGMEM = "bullet.wav";
const char filename_14[] PROGMEM = "dog.wav";
const char filename_15[] PROGMEM = "dial_up.wav";
const char filename_16[] PROGMEM = "futuristic_gunshot.wav";
const char filename_17[] PROGMEM = "";
const char filename_18[] PROGMEM = "glass.wav";
const char filename_19[] PROGMEM = "glass.wav";
const char filename_20[] PROGMEM = "glass.wav";
const char filename_21[] PROGMEM = "glass.wav";
const char filename_22[] PROGMEM = "glass.wav";
const char filename_23[] PROGMEM = "glass.wav";
const char filename_24[] PROGMEM = "glass.wav";
const char filename_25[] PROGMEM = "glass.wav";
const char filename_26[] PROGMEM = "glass.wav";
const char filename_27[] PROGMEM = "glass.wav";
const char filename_28[] PROGMEM = "glass.wav";
const char filename_29[] PROGMEM = "glass.wav";
const char filename_30[] PROGMEM = "glass.wav";
const char filename_31[] PROGMEM = "glass.wav";
const char filename_32[] PROGMEM = "glass.wav";

PGM_P sound_files[NUMBER_OF_SOUNDS] = {
  filename_0,
  filename_1,
  filename_2,
  filename_3,
  filename_4,
  filename_5,
  filename_6,
  filename_7,
  filename_8,
  filename_9,
  filename_10,
  filename_11,
  filename_12,
  filename_13,
  filename_14,
  filename_15,
  filename_16,
  filename_17,
  filename_18,
  filename_19,
  filename_20,
  filename_21,
  filename_22,
  filename_23,
  filename_24,
  filename_25,
  filename_26,
  filename_27,
  filename_28,
  filename_29,
  filename_30,
  filename_31,
  filename_32
};

char *readString(PGM_P flash_ptr)
{
  static char stringbuf[32] = {'\0'};
  strcpy_P(stringbuf, flash_ptr);
  return stringbuf;
}
