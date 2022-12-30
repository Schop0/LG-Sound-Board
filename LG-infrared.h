#include <Arduino.h>

#define IR_QUEUE_SIZE 10

typedef union __attribute__((packed)) {
  uint16_t data;
  struct {
    uint8_t command;
    uint8_t address;
  };
} IrCode_t;
static_assert(2 == sizeof(IrCode_t), "IrCode_t length incorrect");

void irInit();
IrCode_t irDecoder();
static inline void irLedOn()  { bitSet  (DDRD, DDD3); }
static inline void irLedOff() { bitClear(DDRD, DDD3); }
static inline void irLedSet(bool state) { state ? irLedOn() : irLedOff(); }
static inline bool irLedState() { return bitRead(DDRD, DDD3); }

bool fireLaser(unsigned int playerNumber);
void runLasergame();
