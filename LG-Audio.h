#include "pcmConfig.h"
#include <SdFat.h>
#include <TMRpcm.h>
#include <avr/pgmspace.h>

#define NUMBER_OF_SOUND_BANKS 5
#define NUMBER_OF_SOUNDS_PER_BANK 16
#define NUMBER_OF_SOUNDS (NUMBER_OF_SOUND_BANKS * NUMBER_OF_SOUNDS_PER_BANK)

extern SdFat sd;
extern TMRpcm audio;
extern PGM_P sound_files[NUMBER_OF_SOUNDS];

char *readString(PGM_P flash_ptr);
