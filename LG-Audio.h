#include "pcmConfig.h"
#include <SdFat.h>
#include <TMRpcm.h>

#define NUMBER_OF_SOUND_BANKS (getSoundBankCount())
#define NUMBER_OF_SOUNDS_PER_BANK 16
#define NUMBER_OF_SOUNDS (NUMBER_OF_SOUND_BANKS * NUMBER_OF_SOUNDS_PER_BANK)
#define INDEX_FILE_NAME "index.txt"

extern SdFat sd;
extern TMRpcm audio;

void initSoundBankCount();
unsigned int getSoundBankCount();
const char *soundFileFromKey(unsigned int bank, unsigned int key);
const char *soundFileFromIrCode(uint16_t irCode);
