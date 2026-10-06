#ifdef BUILD_SMS

/**
 * @brief Sega Master System input routines
 * @author Thomas Cherryhomes
 * @license gpl v.3
 */

#include "joystick.h"
#include <arch/sms.h>
#include "../platform-specific/input.h"

/*
  A Master System pad is a d-pad and buttons 1 and 2; Pause is on the console
  and arrives as an NMI that toggles crt0's pause_flag.

  Every button is reported as a KEY rather than as a stick bit. readCommonInput()
  returns early whenever readJoystick()'s value changes and only reaches
  getPlatformKey() when it has not, so a button that appeared in both places
  would be seen twice. Keeping every button in getPlatformKey() puts all of
  them behind one edge detector.

    1           RETURN  select: join, play the move, OK
    2           ESCAPE  the in-game menu, and back out of it
    hold 2 + 1  'q'     quit: leave the table / load the lobby
    Pause       'h'     how to play
    Left        'n'     change name   } only the table list acts on these;
    Right       'r'     refresh       } in play they are just the cursor

  Button 2 is reported on RELEASE, so that holding it can turn button 1 into
  quit; button 1, Left and Right report on press. Pause (crt0's NMI) and
  button 2 (pad.asm, on the frame interrupt) are latched, so neither is lost
  to a network call.
*/

extern void padIrq(void);                   /* pad.asm */
extern volatile unsigned char b2Latch;

static unsigned char lastPad;
static unsigned char lastPause;
static unsigned char b2Down;
static unsigned char chorded;
static unsigned char installed;

static unsigned char readPad(void)
{
  return (unsigned char) ~IO_DC & (JOY_DIRS_MASK | JOY_BTN_1_MASK | JOY_BTN_2_MASK);
}

unsigned char readJoystick()
{
  return readPad() & JOY_DIRS_MASK;
}

/* Also called by the on-screen keyboard on its way out, so that the presses
   it consumed are not seen again here. */
void initPlatformKeyboardInput(void)
{
  if (!installed)
  {
    // The handler list is read by the interrupt; add the entry with it off.
#asm
    di
#endasm
    add_raster_int(padIrq);
#asm
    ei
#endasm
    installed = 1;
  }
  lastPad = readPad();
  lastPause = pause_flag;
  b2Down = lastPad & JOY_BTN_2_MASK;
  b2Latch = 0;
  chorded = 0;
}

int getPlatformKey(void)
{
  unsigned char pad = readPad();
  unsigned char pressed = pad & ~lastPad;

  lastPad = pad;
  if (b2Latch)
  {
    b2Latch = 0;
    b2Down = 1;
  }

  if (pause_flag != lastPause)
  {
    lastPause = pause_flag;
    return 'h';
  }

  if (pressed & JOY_BTN_1_MASK)
  {
    if (pad & JOY_BTN_2_MASK)
    {
      chorded = 1;
      return 'q';
    }
    return KEY_RETURN;
  }

  if (b2Down && !(pad & JOY_BTN_2_MASK))
  {
    b2Down = 0;
    if (chorded)
    {
      chorded = 0;
      return 0;
    }
    return KEY_ESCAPE;
  }

  if (pressed & JOY_LEFT_MASK)
    return 'n';
  if (pressed & JOY_RIGHT_MASK)
    return 'r';
  return 0;
}

#endif /* BUILD_SMS */
