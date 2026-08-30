#ifdef BUILD_MSX

/**
 * @brief   Joystick macros
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 * @verbose {verbose}
 */

#ifndef _JOYSTICK_H
#define _JOYSTICK_H

/*****************************************************************************/
/*                              Joystick Definitions                         */
/*****************************************************************************/

/* These match the bit layout of z88dk's st_dir[] table, which maps the raw
   GTSTCK direction code (0-8) onto a bitmask. See src/msx/joyread.asm. */

#define JOY_UP_MASK             0x01
#define JOY_RIGHT_MASK          0x02
#define JOY_DOWN_MASK           0x04
#define JOY_LEFT_MASK           0x08
#define JOY_BTN_1_MASK          0x10
#define JOY_BTN_2_MASK          0x20

/* Macros that evaluate the return code of readJoystick() */
#define JOY_UP(v)               ((v) & JOY_UP_MASK)
#define JOY_DOWN(v)             ((v) & JOY_DOWN_MASK)
#define JOY_LEFT(v)             ((v) & JOY_LEFT_MASK)
#define JOY_RIGHT(v)            ((v) & JOY_RIGHT_MASK)
#define JOY_BTN_1(v)            ((v) & JOY_BTN_1_MASK)
#define JOY_BTN_2(v)            ((v) & JOY_BTN_2_MASK)

/* End of joystick.h */
#endif /* _JOYSTICK_H */

#endif /* BUILD_MSX */
