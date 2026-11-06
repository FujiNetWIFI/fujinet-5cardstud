#ifdef __H89__

#ifndef KEYMAP_H
#define KEYMAP_H

// Screen dimensions for platform: the H19 terminal's 80x24, plus its 25th
// line (row 24 = HEIGHT-1), which src/h89/console.c turns on for the status
// bar. Not SINGLE_BUFFER: the table is repainted in full every poll, into a
// back buffer, and only the cells that differ go down the 9600 baud line.
#define WIDTH 80
#define HEIGHT 25

// Cards are 5x5 (src/h89/graphics.c), every 4 columns, each sharing its
// right border with the next: the shared code's card offsets (0,1,3,5,7, or
// 0,2,4,6,8 spread) count in steps of two columns here, so a face-down hole
// card shows as a two column sliver. Opponents' hands are 19 wide, YOU's 21.
// A right-side seat's name and purse end at its card's right edge (CARD_W-1).
#define CARD_W 5
#define CARD_SPREAD(n) ((n)*2)

// The table list: double spaced like the other platforms' while it fits
// between the header and the frame's bottom edge (row 23), one table a row
// past eight. (Expanded where screens.c lays the list out, which can see
// clientState.)
#define TABLE_TOP 5
#define TABLE_ROW_HEIGHT (clientState.tables.count > 8 ? 1 : 2)

#define QUERY_SUFFIX ""

// The H19 sends its keypad and function keys as escape sequences, so
// src/h89/input.c decodes the keyboard itself, without blocking.
#define USE_PLATFORM_SPECIFIC_INPUT 1

/**
 * Platform specific key map for common input (H19 keyboard)
 */

// src/h89/input.c turns the keypad's ESC A-D (keypad shifted mode, set by
// termInit) into the MSX's cursor codes, which stay below 0x80 because
// inputKey is a plain char. The _2 codes are unreachable placeholders --
// readCommonInput() switches on all twelve -- and the _3 keys are spares for
// a keyboard without the keypad.
#define KEY_LEFT_ARROW      0x1D
#define KEY_LEFT_ARROW_2    0xF1
#define KEY_LEFT_ARROW_3    0x2C // ,

#define KEY_RIGHT_ARROW     0x1C
#define KEY_RIGHT_ARROW_2   0xF2
#define KEY_RIGHT_ARROW_3   0x2E // .

#define KEY_UP_ARROW        0x1E
#define KEY_UP_ARROW_2      0xF3
#define KEY_UP_ARROW_3      0x2D // -

#define KEY_DOWN_ARROW      0x1F
#define KEY_DOWN_ARROW_2    0xF4
#define KEY_DOWN_ARROW_3    0x3D // =

#define KEY_RETURN       0x0D
#define KEY_ESCAPE       0x1B
#define KEY_ESCAPE_ALT   0x03 // CTRL-C
#define KEY_SPACE        0x20
#define KEY_BACKSPACE    0x08

/* Keyboard only: readJoystick() always returns 0 */
#define JOY_UP(v)    (0)
#define JOY_DOWN(v)  (0)
#define JOY_LEFT(v)  (0)
#define JOY_RIGHT(v) (0)
#define JOY_BTN_1(v) (0)

/*
  Mapping for converting incoming ALT letters to a standard case
*/
#define LINE_ENDING 0x0A
#define ALT_LETTER_START 0x0
#define ALT_LETTER_END 0x0
#define ALT_LETTER_AND 0x0

/*
 Screen related variables
*/

// Screen specific player/bet coordinates (src/h89/vars.c)
extern unsigned char playerXMaster[];
extern unsigned char playerYMaster[];

extern char playerDirMaster[];
extern char playerBetXMaster[];
extern char playerBetYMaster[];

// Simple hard coded arrangment of players around the table based on player count.
// These refer to index positions in the Master arrays above
// Downside is new players will cause existing player positions to move.
extern char playerCountIndex[];

#endif /* KEYMAP_H */

#endif /* __H89__ */
