#ifdef __ADAM__

/**
 * @brief Adam input routines
 * @author Thomas Cherryhomes, Geoff Oltmans
 * @license gpl v.3
 */

#include <eos.h>
#include "../platform-specific/input.h"

static GameControllerData cont;

unsigned char readJoystick()
{
  unsigned char temp = 0;
  eos_read_game_controller(0x03,&cont);
  temp = cont.joystick1;
  temp |= (cont.joystick1_button_left || cont.joystick1_button_right)<<4;

  return temp;
}

void initPlatformKeyboardInput(void)
{
  eos_start_read_keyboard();
}

int getPlatformKey(void)
{
  int ch;
  ch=eos_end_read_keyboard();

  // <=1 means the asynchronous read is still pending (no key yet).
  if (ch>1)
  {
    eos_start_read_keyboard();
    return ch;
  }
  return 0;
}

#endif /* __ADAM__ */
