#include "LG-Audio.h"

SdFat sd;
TMRpcm audio;

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
