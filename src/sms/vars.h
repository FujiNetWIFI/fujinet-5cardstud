#ifdef BUILD_SMS

#ifndef KEYMAP_H
#define KEYMAP_H

// Screen dimensions for platform. Mode 4 shows exactly 32x24 tiles, the same
// layout the ColecoVision, Adam and MSX ports share.

#define WIDTH 32
#define HEIGHT 24

#define SINGLE_BUFFER 1

#define POT_Y_MODIFIER -1
#define STATUS_TIMER_WIDTH 0
#define HOW_TO_PLAY_ROW_START 2

// There is no keyboard: a d-pad, buttons 1 and 2, and Pause on the console.
// src/sms/input.c synthesises key codes from them and src/sms/osk.c types
// text on screen.
#define USE_PLATFORM_SPECIFIC_INPUT 1
#define USE_PLATFORM_NAME_ENTRY 1

/**
 * Platform specific key map for common input
 */

// Direction comes from the d-pad, never from these codes, so they are
// deliberately unreachable placeholders -- they exist because
// readCommonInput() switches on all twelve of them.
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

// Button 1 and button 2.
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

#endif /* KEYMAP_H */

#endif /* BUILD_SMS */
