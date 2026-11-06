#ifdef __TRS80M2__

#include <stdint.h>
#include <stdlib.h>
#include "con.h"
#include "../platform-specific/graphics.h"
#include "../platform-specific/util.h"

static uint32_t startTicks;

void resetTimer() {
  startTicks = con_ticks();
}

// 60ths of a second from P&T's 1/100 s clock
int getTime() {
  return (int)((con_ticks() - startTicks) * 3 / 5);
}

// There is no Model II lobby: back to CP/M
void quit() {
  resetGraphics();
  exit(0);
}

#endif /* __TRS80M2__ */
