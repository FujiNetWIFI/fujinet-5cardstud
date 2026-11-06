#ifdef BUILD_ATARI7800

#ifndef KEYMAP_H
#define KEYMAP_H

// Screen dimensions for platform: the shared MARIA engine's 32x24 cells.

#define WIDTH 32
#define HEIGHT 24

#define SINGLE_BUFFER 1

#define POT_Y_MODIFIER -1
#define STATUS_TIMER_WIDTH 0
#define HOW_TO_PLAY_ROW_START 2

// There is no keyboard: a joystick with one or two buttons, and the console's
// RESET, SELECT and PAUSE. src/atari7800/input.c synthesises key codes from
// them and src/atari7800/osk.c types text on screen.
#define USE_PLATFORM_SPECIFIC_INPUT 1
#define USE_PLATFORM_NAME_ENTRY 1

/**
 * Platform specific key map for common input
 */

// Direction comes from the joystick, never from a key, so the arrow codes are
// deliberately unreachable placeholders -- they exist because readCommonInput()
// switches on all twelve of them.
#define KEY_LEFT_ARROW      0xF1
#define KEY_LEFT_ARROW_2    0xF2
#define KEY_LEFT_ARROW_3    0xF3

#define KEY_RIGHT_ARROW     0xF4
#define KEY_RIGHT_ARROW_2   0xF5
#define KEY_RIGHT_ARROW_3   0xF6

#define KEY_UP_ARROW        0xF7
#define KEY_UP_ARROW_2      0xF8
#define KEY_UP_ARROW_3      0xF9

#define KEY_DOWN_ARROW      0xFA
#define KEY_DOWN_ARROW_2    0xFB
#define KEY_DOWN_ARROW_3    0xFC

// Buttons 1 and 2.
#define KEY_RETURN       0x0D
#define KEY_ESCAPE       0x1B
#define KEY_ESCAPE_ALT   0x03
#define KEY_SPACE        0x20
#define KEY_BACKSPACE    0x08

/*
  Mapping for converting incoming ALT letters to a standard case
*/
#define LINE_ENDING 0x0A
#define ALT_LETTER_START 0x0
#define ALT_LETTER_END 0x0
#define ALT_LETTER_AND 0x0

#define QUERY_SUFFIX ""

/*
 The controls, as readPad() reports them: the joystick in cc65's
 JOY_*_MASK bits, button 2 including RESET, and the two other switches.
*/
#define PAD_SELECT 0x04
#define PAD_PAUSE  0x08

unsigned char readPad(void);

// Frames seen by waitvsync(), for the move timer.
extern unsigned int frameCount;

// A palette changed; it shows from the next waitvsync().
extern unsigned char palDirty;
void waitvsync(void);

/*
 Screen related variables
*/

// Screen specific player/bet coordinates, in ROM.
extern const unsigned char playerXMaster[];
extern const unsigned char playerYMaster[];
extern const char playerDirMaster[];
extern const char playerBetXMaster[];
extern const char playerBetYMaster[];

// Simple hard coded arrangment of players around the table based on player count.
// These refer to index positions in the Master arrays above
// Downside is new players will cause existing player positions to move.
extern const char playerCountIndex[];

#endif /* KEYMAP_H */

#endif /* BUILD_ATARI7800 */
