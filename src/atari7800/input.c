#ifdef BUILD_ATARI7800

/**
 * @brief Atari 7800 input routines
 * @author Thomas Cherryhomes
 * @license gpl v.3
 */

#include <joystick.h>
#include "../platform-specific/input.h"

/*
  The left joystick and the console's switches. A ProLine joystick is read in
  two-button mode (driven from the RIOT only); a 2600 joystick's one button
  arrives through INPT4, and RESET stands in for its missing button 2.

  Every button is reported as a KEY rather than as a stick bit. readCommonInput()
  returns early whenever readJoystick()'s value changes and only reaches
  getPlatformKey() when it has not, so a button that appeared in both places
  would be seen twice. Keeping every button in getPlatformKey() puts all of
  them behind one edge detector.

    1              RETURN  (select / join / confirm)
    2 or RESET     ESCAPE  (back / the in-game menu)
    PAUSE          'h'     how to play
    SELECT         'n'     change name
    SELECT+PAUSE   'q'     quit the table

  PAUSE and SELECT are reported on RELEASE so that the chord can be told from
  either switch alone. The buttons report on press.
*/

#define REG(a)  (*(volatile unsigned char *)(a))
#define INPT0   REG(0x08)       /* button 2, two-button mode: bit 7 set */
#define INPT1   REG(0x09)       /* button 1, two-button mode: bit 7 set */
#define INPT4   REG(0x0C)       /* a 2600 joystick's button: bit 7 clear */
#define SWCHA   REG(0x280)      /* bits 7-4: right, left, down, up; low */
#define SWCHB   REG(0x282)      /* bit 0 RESET, 1 SELECT, 3 PAUSE; low */
#define CTLSWB  REG(0x283)

#define PB2     0x04            /* the left port's two-button mode, driven low */

#define DIRS (JOY_UP_MASK | JOY_DOWN_MASK | JOY_LEFT_MASK | JOY_RIGHT_MASK)
#define MODS (PAD_SELECT | PAD_PAUSE)

static unsigned char lastButtons;
static unsigned char modsSeen;

unsigned char readPad(void)
{
  unsigned char pad = (unsigned char)~SWCHA & DIRS;
  unsigned char sw = (unsigned char)~SWCHB;

  if ((INPT1 & 0x80) || !(INPT4 & 0x80))
    pad |= JOY_BTN_1_MASK;
  if ((INPT0 & 0x80) || (sw & 0x01))
    pad |= JOY_BTN_2_MASK;
  if (sw & 0x02)
    pad |= PAD_SELECT;
  if (sw & 0x08)
    pad |= PAD_PAUSE;
  return pad;
}

unsigned char readJoystick()
{
  // Palette changes show only after a frame: let the last ones land before
  // the game sits waiting for input.
  if (palDirty)
    waitvsync();
  return readPad() & DIRS;
}

void initPlatformKeyboardInput(void)
{
  CTLSWB = PB2;
  SWCHB = 0;
  lastButtons = readPad() & ~DIRS;
  modsSeen = 0;
}

int getPlatformKey(void)
{
  unsigned char buttons = readPad() & ~DIRS;
  unsigned char pressed = buttons & ~lastButtons;
  unsigned char released = lastButtons & ~buttons;

  lastButtons = buttons;
  modsSeen |= buttons & MODS;

  if (pressed & JOY_BTN_1_MASK)
    return KEY_RETURN;
  if (pressed & JOY_BTN_2_MASK)
    return KEY_ESCAPE;

  if ((released & MODS) && !(buttons & MODS)) {
    unsigned char chord = modsSeen;
    modsSeen = 0;
    if (chord == MODS)
      return 'q';
    if (chord == PAD_PAUSE)
      return 'h';
    if (chord == PAD_SELECT)
      return 'n';
  }
  return 0;
}

#endif /* BUILD_ATARI7800 */
