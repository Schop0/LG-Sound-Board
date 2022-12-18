#include <Arduino.h>

#define IR_QUEUE_SIZE 10

void irInit();
unsigned long int irDecoder();
static inline void irLedOn()  { bitSet  (DDRD, DDD3); }
static inline void irLedOff() { bitClear(DDRD, DDD3); }
static inline void irLedSet(bool state) { state ? irLedOn() : irLedOff(); }
static inline bool irLedState() { return bitRead(DDRD, DDD3); }
