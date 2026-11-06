#ifdef __TRS80M2__

// The console bell is the only sound

#include "con.h"
#include "vid.h"
#include "../misc.h"
#include "../platform-specific/sound.h"

// 1/100 s between bells, so they don't run together
#define BELL_GAP 25

extern unsigned char moveTimeLeft;

static void bells(uint8_t n) {
  static uint32_t start;

  vid_flush();
  while (n--) {
    vid_bell();
    if (n) {
      start = con_ticks();
      while (con_ticks() - start < BELL_GAP)
        ;
    }
  }
}

void initSound() {}

void soundJoinGame() {
  bells(1);
}

void soundMyTurn() {
  bells(2);
}

void soundGameDone() {
  bells(3);
}

void soundTick() {
  if (moveTimeLeft <= 5)
    bells(1);
}

void soundCursorInvalid() {
  bells(1);
}

void soundDealCard() {}
void soundPlayerJoin() {}
void soundPlayerLeft() {}
void soundSelectMove() {}
void soundCursor() {}
void soundTakeChip(uint16_t counter) { (void)counter; }

#endif /* __TRS80M2__ */
