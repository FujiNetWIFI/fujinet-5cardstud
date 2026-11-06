#ifdef __TRS80M2__

#ifndef KEYMAP_H
#define KEYMAP_H

#define WIDTH 80
#define HEIGHT 24
#define STATUS_TIMER_WIDTH 0
#define NO_COLOR
#define MIXED_CASE

#define CARD_W 4
#define CARD_SPREAD(n) ((n) * 2)

#define QUERY_SUFFIX ""

#define TABLE_HINTS "R)efresh   H)elp   N)ame   Q)uit"

// P&T CP/M key codes; F1 and F2 are spare up and down keys
#define KEY_LEFT_ARROW      0x1C
#define KEY_LEFT_ARROW_2    ','
#define KEY_LEFT_ARROW_3    '<'

#define KEY_RIGHT_ARROW     0x1D
#define KEY_RIGHT_ARROW_2   '.'
#define KEY_RIGHT_ARROW_3   '>'

#define KEY_UP_ARROW        0x1E
#define KEY_UP_ARROW_2      '-'
#define KEY_UP_ARROW_3      0x01

#define KEY_DOWN_ARROW      0x1F
#define KEY_DOWN_ARROW_2    '='
#define KEY_DOWN_ARROW_3    0x02

#define KEY_RETURN       0x0D
#define KEY_ESCAPE       0x1B
#define KEY_ESCAPE_ALT   0x03
#define KEY_SPACE        0x20
#define KEY_BACKSPACE    0x08

#define JOY_UP(v)     (0)
#define JOY_DOWN(v)   (0)
#define JOY_LEFT(v)   (0)
#define JOY_RIGHT(v)  (0)
#define JOY_BTN_1(v)  (0)

// Raw BIOS console, so BREAK reaches the game
#undef cgetc
#define cgetc m2Cgetc
#define kbhit m2Kbhit
unsigned char m2Kbhit(void);
unsigned char m2Cgetc(void);

#define drawTextCursor m2TextCursor
void m2TextCursor(unsigned char x, unsigned char y);

#define drawActiveName m2ActiveName
void m2ActiveName(unsigned char x, unsigned char y, unsigned char w);

#endif /* KEYMAP_H */

#endif /* __TRS80M2__ */
