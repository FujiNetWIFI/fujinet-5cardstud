#ifdef __TRS80M2__

#include "con.h"
#include "vid.h"
#include "../platform-specific/input.h"

static int pending = -1;

unsigned char readJoystick() {
  return 0;
}

// The screen is sent whenever the game looks for a key
unsigned char m2Kbhit(void) {
  vid_flush();
  if (pending < 0)
    pending = con_in();
  return pending >= 0;
}

unsigned char m2Cgetc(void) {
  static unsigned char c;

  while (!m2Kbhit())
    ;
  c = (unsigned char)pending;
  pending = -1;
  return c;
}

#endif /* __TRS80M2__ */
