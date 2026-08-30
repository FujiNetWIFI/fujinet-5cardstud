#ifdef BUILD_MSX

#ifndef KEYMAP_H
#define KEYMAP_H

// Screen dimensions for platform

#define WIDTH 32
#define HEIGHT 24

#define SINGLE_BUFFER 1

#define POT_Y_MODIFIER -1
#define STATUS_TIMER_WIDTH 0
#define HOW_TO_PLAY_ROW_START 2

/**
 * Platform specific key map for common input
 */

// Primary codes are what the MSX BIOS CHGET actually returns for the cursor
// keys. The _2 slots are unused placeholders (readCommonInput() switches on all
// three, so they only have to be distinct and unreachable). The _3 slots are
// the , . - = alternates shared with the other ports.

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
 Screen related variables
*/

// Screen specific player/bet coordinates
extern unsigned char playerXMaster[];
extern unsigned char playerYMaster[];

extern char playerDirMaster[];
extern char playerBetXMaster[];
extern char playerBetYMaster[];

// Simple hard coded arrangment of players around the table based on player count.
// These refer to index positions in the Master arrays above
// Downside is new players will cause existing player positions to move.

//                               2                3                4
extern char playerCountIndex[];


#endif /* KEYMAP_H */

#endif /* BUILD_MSX */
