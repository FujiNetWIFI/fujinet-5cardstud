#ifdef BUILD_SMS

/**
 * @brief   Sega Master System Graphics Routines for 5cardstud
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 * @verbose VDP Mode 4, 32x24 tiles. The art is the MS-DOS CGA set with its
 *          colours baked in (src/sms/mktiles.py); palette 1 turns the felt
 *          black for the status bar and the card face grey for the viewer's
 *          hidden hole card. A shadow of the glyph codes stands in for the
 *          TMS9918 ports' cvpeek().
 */

#include <stdbool.h>
#include <string.h>
#include "tiles.h"
#include "vars.h"
#include "../platform-specific/graphics.h"

#define CORNER_TOP 0
#define CORNER_BOTTOM 18

#define NAME_TABLE  0x3800
#define SAT         0x3F00
#define VRAM_WRITE  0x4000
#define CRAM_WRITE  0xC000
#define STATUS_ROW  (HEIGHT - 1)
#define TALL_ROW    (HEIGHT - 2)

/* Name table entry bits above the tile number. */
#define PAL1        0x0800

/* Text styles: the name table entry for glyph c is style + c. */
#define SPACE       0x20
#define TEXT        0x0000          /* white on felt */
#define STATUS      PAL1            /* white on black */
#define HILITE      (T_HILITE - 0x20)   /* black on gold */

extern void vdp_addr(unsigned int cmd) __z88dk_fastcall;   /* vdp.asm */
extern void vdp_word(unsigned int w) __z88dk_fastcall;
extern void vdp_byte(unsigned char b) __z88dk_fastcall;
extern void vdp_tiles(const unsigned char *runs) __z88dk_fastcall;

bool always_render_full_cards = 0;

/* What each cell shows, as a glyph code (ASCII or UDG_*), and the name table
   entry drawing it. */
static unsigned char glyphs[HEIGHT][WIDTH];
static unsigned int cells[HEIGHT][WIDTH];

/* A two-line status message borrows row 22. While it does, row 22's own
   drawing lands here instead and comes back when the bar is cleared. */
static bool tallStatus;
static unsigned char underGlyphs[WIDTH];
static unsigned int underCells[WIDTH];

static void vdp_reg(unsigned char r, unsigned char v)
{
    vdp_addr(((unsigned int)(0x80 | r) << 8) | v);
}

static void seek(unsigned char x, unsigned char y)
{
    vdp_addr(VRAM_WRITE | NAME_TABLE | ((((unsigned int)y << 5) + x) << 1));
}

/* Write a cell straight to the screen, bypassing the row 22 underlay. */
static void poke(unsigned char x, unsigned char y, unsigned char glyph, unsigned int cell)
{
    glyphs[y][x] = glyph;
    cells[y][x] = cell;
    seek(x, y);
    vdp_word(cell);
}

static void put(unsigned char x, unsigned char y, unsigned char glyph, unsigned int cell)
{
    if (x >= WIDTH || y >= HEIGHT)
        return;
    if (y == TALL_ROW && tallStatus)
    {
        underGlyphs[x] = glyph;
        underCells[x] = cell;
        return;
    }
    poke(x, y, glyph, cell);
}

static unsigned char peek(unsigned char x, unsigned char y)
{
    if (x >= WIDTH || y >= HEIGHT)
        return ' ';
    if (y == TALL_ROW && tallStatus)
        return underGlyphs[x];
    return glyphs[y][x];
}

/* Fold to the font's upper-case range; anything else prints as '?'. */
static unsigned char textGlyph(unsigned char c)
{
    if (c >= 'a' && c <= 'z')
        c -= 'a' - 'A';
    if (c < 0x20 || c > 0x5F)
        c = '?';
    return c;
}

static void putText(unsigned char x, unsigned char y, const char *s, unsigned int style)
{
    unsigned char c;

    while ((c = (unsigned char) *s++) != 0 && x < WIDTH)
    {
        c = textGlyph(c);
        put(x++, y, c, style + c);
    }
}

/**
 * @brief Initialize graphics mode; set palette.
 */
void initGraphics()
{
    unsigned char i;

    vdp_reg(1, 0x80);               // display and frame interrupt off
    vdp_reg(7, 0x00);               // backdrop: palette 1 colour 0, black

    vdp_addr(CRAM_WRITE);
    for (i = 0; i < PAL_COUNT; i++)
        vdp_byte(sms_palette[i]);
    vdp_addr(CRAM_WRITE | 16);
    for (i = 0; i < PAL_COUNT; i++)
        vdp_byte(sms_palette[PAL_COUNT + i]);

    vdp_tiles(sms_tileset);

    // No sprites: a Y of $D0 ends the list at the first entry.
    vdp_addr(VRAM_WRITE | SAT);
    vdp_byte(0xD0);

    resetScreen();

    // Display on, frame interrupt on: crt0's handler counts frames in timer,
    // which waitvsync() and getTime() read (src/sms/util.c).
    vdp_reg(1, 0xE0);
    waitvsync();
}

void drawChip(unsigned char x, unsigned char y)
{
    put(x, y, UDG_CHIP, UDG_CHIP);
}

void drawBuffer()
{
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned char i;
    // Correct coordinates;
    w++;
    h++;

    // Put box corners at coordinate extents
    put(x, y, UDG_BOX_TL, UDG_BOX_TL);
    put(x + w, y, UDG_BOX_TR, UDG_BOX_TR);
    put(x, y + h, UDG_BOX_BL, UDG_BOX_BL);
    put(x + w, y + h, UDG_BOX_BR, UDG_BOX_BR);

    // Horizontal rules
    for (i = 1; i < w; i++)
    {
        put(x + i, y, UDG_BOX_H, UDG_BOX_H);
        put(x + i, y + h, UDG_BOX_H, UDG_BOX_H);
    }

    // Vertical rules
    for (i = 1; i < h; i++)
    {
        put(x, y + i, UDG_BOX_V, UDG_BOX_V);
        put(x + w, y + i, UDG_BOX_V, UDG_BOX_V);
    }
}

/**
 * @brief Draw text s at position x,y
 * @param x Horizontal position (0-31)
 * @param y Vertical Position (0-23)
 * @param s NULL terminated string to display.
 */
void drawText(unsigned char x, unsigned char y, const char* s)
{
    putText(x, y, s, TEXT);
}

/* Black on gold: the on-screen keyboard's cursor (src/sms/osk.c). */
void drawHiliteText(unsigned char x, unsigned char y, const char* s)
{
    putText(x, y, s, HILITE);
}

/**
 * @brief Draw the 5 Card Stud Logo, as a gold plaque on the felt.
 */
void drawLogo()
{
    unsigned char i = 4;
    putText(WIDTH/2-6,++i, "             ", HILITE);
    putText(WIDTH/2-6,++i, "  FUJI  NET  ", HILITE);
    putText(WIDTH/2-6,++i, "             ", HILITE);
    putText(WIDTH/2-6,++i, " 5 CARD STUD ", HILITE);
    putText(WIDTH/2-6,++i, "             ", HILITE);
}

static void blankRow(unsigned char y, unsigned int cell)
{
    unsigned char x;

    seek(0, y);
    for (x = 0; x < WIDTH; x++)
    {
        glyphs[y][x] = ' ';
        cells[y][x] = cell;
        vdp_word(cell);
    }
}

void clearStatusBar()
{
    unsigned char x;

    blankRow(STATUS_ROW, STATUS | SPACE);
    if (tallStatus)
    {
        // Give row 22 back what was drawn under the message.
        tallStatus = false;
        seek(0, TALL_ROW);
        for (x = 0; x < WIDTH; x++)
        {
            glyphs[TALL_ROW][x] = underGlyphs[x];
            cells[TALL_ROW][x] = underCells[x];
            vdp_word(underCells[x]);
        }
    }
}

/**
 * @brief Clear the screen
 */
void resetScreen()
{
    unsigned char y;

    tallStatus = false;
    for (y = 0; y < STATUS_ROW; y++)
        blankRow(y, TEXT | SPACE);
    clearStatusBar();

    // Round the felt off against the border/status bar, like the MS-DOS build.
    put(0, 0, UDG_SCREEN_TL, UDG_SCREEN_TL);
    put(WIDTH-1, 0, UDG_SCREEN_TR, UDG_SCREEN_TR);
    put(0, 22, UDG_SCREEN_BL, UDG_SCREEN_BL);
    put(WIDTH-1, 22, UDG_SCREEN_BR, UDG_SCREEN_BR);
}

void disableDoubleBuffer()
{
}

void enableDoubleBuffer()
{
}

/**
 * @brief Draw card at position x,y
 * @param x Horizontal card position (0-31)
 * @param y Vertical card position (0-23)
 * @param partial enum (see ../platform-specific/graphics.h)
 * @param s String indicating number and suit. (e.g. "as" for ace of spades)
 * @param isHidden is card currently overturned?
 */
void drawCard(unsigned char x, unsigned char y, unsigned char partial, const char* s, unsigned char isHidden)
{
    unsigned char val, suit, i;
    unsigned int rank, face;

    if (x==WIDTH-3 && s[0]!='?' && peek(x,y+1)==UDG_BACK_L_TOP) {
        drawCard(x+1,y,PARTIAL_RIGHT,"??",false);
    }

    if (partial == PARTIAL_LEFT)
    {
        put(x, y, UDG_CARD_TL, UDG_CARD_TL);
        put(x, y+1, UDG_BACK_L_TOP, UDG_BACK_L_TOP);
        put(x, y+2, UDG_BACK_L_MID, UDG_BACK_L_MID);
        put(x, y+3, UDG_BACK_L_BOT, UDG_BACK_L_BOT);
        put(x, y+4, UDG_CARD_BL, UDG_CARD_BL);
    }
    else if (partial == PARTIAL_RIGHT)
    {
        x++;
        put(x, y, UDG_CARD_TOP_TRIM, UDG_CARD_TOP_TRIM);
        put(x, y+1, UDG_BACK_RCOL_TOP, UDG_BACK_RCOL_TOP);
        put(x, y+2, UDG_BACK_RCOL_MID, UDG_BACK_RCOL_MID);
        put(x, y+3, UDG_BACK_RCOL_BOT, UDG_BACK_RCOL_BOT);
        put(x, y+4, UDG_CARD_BOT_TRIM, UDG_CARD_BOT_TRIM);
    }
    else if (s[0]=='?') // FULL CARD, overturned: draw the back
    {
        // Shift right card left one for easy drawing of border
        // As well as clear existing cards, assuming a fold
        if (x>WIDTH-3) {
            for (i=0;i<5;i++)
                drawText(x-7,y+i,"      ");
            x--;
        } else {
            for (i=0;i<5;i++)
                drawText(x+3,y+i,"       ");
        }
        put(x, y, UDG_CARD_TL, UDG_CARD_TL);
        put(x+1, y, UDG_CARD_TOP, UDG_CARD_TOP);
        put(x+2, y, UDG_CARD_TR_STUB, UDG_CARD_TR_STUB);
        put(x, y+1, UDG_BACK_L_TOP, UDG_BACK_L_TOP);
        put(x+1, y+1, UDG_BACK_R_TOP, UDG_BACK_R_TOP);
        put(x+2, y+1, UDG_CARD_VERT, UDG_CARD_VERT);
        put(x, y+2, UDG_BACK_L_MID, UDG_BACK_L_MID);
        put(x+1, y+2, UDG_BACK_R_MID, UDG_BACK_R_MID);
        put(x+2, y+2, UDG_CARD_VERT, UDG_CARD_VERT);
        put(x, y+3, UDG_BACK_L_BOT, UDG_BACK_L_BOT);
        put(x+1, y+3, UDG_BACK_R_BOT, UDG_BACK_R_BOT);
        put(x+2, y+3, UDG_CARD_VERT, UDG_CARD_VERT);
        put(x, y+4, UDG_CARD_BL, UDG_CARD_BL);
        put(x+1, y+4, UDG_CARD_BOT, UDG_CARD_BOT);
        put(x+2, y+4, UDG_CARD_BR_STUB, UDG_CARD_BR_STUB);
    }
    else // FULL CARD, face up
    {
        // The viewer's hole card, which nobody else can see, has a grey face.
        face = isHidden ? PAL1 : 0;

        switch (s[1])
        {
        case 'h' :
            suit=UDG_SUIT_HEART;
            break;
        case 'd' :
            suit=UDG_SUIT_DIAMOND;
            break;
        case 'c' :
            suit=UDG_SUIT_CLUB;
            break;
        case 's' :
            suit=UDG_SUIT_SPADE;
            break;
        default:
            suit=UDG_CARD_VERT;         // Something wrong.
            break;
        }

        switch (s[0])
        {
        case 't':
            val=UDG_RANK_T; // 10
            break;
        case 'j':
            val=UDG_RANK_J;
            break;
        case 'q':
            val=UDG_RANK_Q;
            break;
        case 'k':
            val=UDG_RANK_K;
            break;
        case 'a':
            val=UDG_RANK_A;
            break;
        default:
            val=UDG_RANK_2+(s[0]-'2');
            break;
        }
        rank = (s[1]=='h' || s[1]=='d') ? T_RANK_RED + (val - UDG_RANK_2) : val;

        // Top card border
        if (peek(x+1,y)!=UDG_CARD_TOP)
        {
            put(x, y, UDG_CARD_TL, UDG_CARD_TL);
            put(x+1, y, UDG_CARD_TOP, UDG_CARD_TOP);
        }
        if (peek(x+2,y)==' ')
            put(x+2, y, UDG_CARD_TR_STUB, UDG_CARD_TR_STUB);

        // Left border and card value
        put(x, y+1, UDG_CARD_VERT, T_CARD_VERT_FACE | face);
        put(x+1, y+1, val, rank | face);
        if (peek(x+2,y+1)==' ')
            put(x+2, y+1, UDG_CARD_VERT, UDG_CARD_VERT);

        // Interior
        put(x, y+2, UDG_CARD_VERT, T_CARD_VERT_FACE | face);
        put(x+1, y+2, ' ', T_BLANK_FACE | face);
        if (peek(x+2,y+2)==' ')
            put(x+2, y+2, UDG_CARD_VERT, UDG_CARD_VERT);

        // Suit
        put(x, y+3, UDG_CARD_VERT, T_CARD_VERT_FACE | face);
        put(x+1, y+3, suit, suit | face);
        if (peek(x+2,y+3)==' ')
            put(x+2, y+3, UDG_CARD_VERT, UDG_CARD_VERT);

        // Bottom border
        if (peek(x+1,y+4)!=UDG_CARD_BOT)
        {
            put(x, y+4, UDG_CARD_BL, UDG_CARD_BL);
            put(x+1, y+4, UDG_CARD_BOT, UDG_CARD_BOT);
        }
        if (peek(x+2,y+4)==' ')
            put(x+2, y+4, UDG_CARD_BR_STUB, UDG_CARD_BR_STUB);
    }
}

/* Up to n characters of s on status row y, white on black. */
static void putStatus(unsigned char x, unsigned char y, const char *s, unsigned char n)
{
    unsigned char c;

    while (n-- && (c = (unsigned char) *s++) != 0 && x < WIDTH)
    {
        c = textGlyph(c);
        poke(x++, y, c, STATUS + c);
    }
}

/* A message too long for one row (the round result) takes row 22 into the
   bar as well, word-wrapped across the two. */
void drawStatusTextAt(unsigned char x, const char* s)
{
    unsigned char brk, i;

    if (strlen(s) <= WIDTH)
    {
        putStatus(x, STATUS_ROW, s, WIDTH);
        return;
    }

    for (brk = WIDTH; brk > 0 && s[brk] != ' '; brk--) ;
    if (brk == 0)
        brk = WIDTH;

    if (!tallStatus)
    {
        memcpy(underGlyphs, glyphs[TALL_ROW], WIDTH);
        memcpy(underCells, cells[TALL_ROW], sizeof underCells);
        tallStatus = true;
    }
    for (i = 0; i < WIDTH; i++)
    {
        poke(i, TALL_ROW, ' ', STATUS | SPACE);
        poke(i, STATUS_ROW, ' ', STATUS | SPACE);
    }
    putStatus(0, TALL_ROW, s, brk);
    putStatus(0, STATUS_ROW, s + brk + (s[brk] == ' '), WIDTH);
}

void drawStatusText(const char* s)
{
    clearStatusBar();
    drawStatusTextAt(0, s);
}

unsigned char cycleNextColor()
{
    return 0;
}

void drawStatusTimer()
{
}

/* The barred tile for a cell off the status row, or 0 if it has none. */
static unsigned int barTile(unsigned char x, unsigned char y)
{
    unsigned int cell = cells[y][x];

    switch (glyphs[y][x])
    {
    case ' ':
        if (cell != (TEXT | SPACE))
            return 0;
        break;
    case UDG_CARD_TL:
    case UDG_CARD_TOP:
    case UDG_CARD_TR_STUB:
    case UDG_CARD_TOP_TRIM:
        break;
    default:
        return 0;
    }
    return T_TOPBAR | glyphs[y][x];
}

/* On the status row the move under the cursor turns black on gold; off it, a
   gold bar runs across the top of the row (under the active player's name).
   Hiding puts back what the cell holds. */
static void line(unsigned char x, unsigned char y, unsigned char w, bool on)
{
    unsigned int cell;

    if (y >= HEIGHT || (y == TALL_ROW && tallStatus))
        return;
    for (; w != 0 && x < WIDTH; w--, x++)
    {
        cell = cells[y][x];
        if (on)
        {
            if (y == STATUS_ROW)
                cell = HILITE + glyphs[y][x];
            else if (barTile(x, y))
                cell = barTile(x, y);
        }
        seek(x, y);
        vdp_word(cell);
    }
}

void hideLine(unsigned char x, unsigned char y, unsigned char w)
{
    line(x, y, w, false);
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    line(x, y, w, true);
}

void setColorMode(unsigned char mode)
{
}

void drawBorder()
{
    drawCard(1,CORNER_TOP,FULL_CARD, "as", 0);
    drawCard(WIDTH-3,CORNER_TOP,FULL_CARD, "ah", 0);
    drawCard(1,CORNER_BOTTOM,FULL_CARD, "ad", 0);
    drawCard(WIDTH-3,CORNER_BOTTOM,FULL_CARD, "ac", 0);
}

#endif /* BUILD_SMS */
