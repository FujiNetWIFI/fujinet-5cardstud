#ifdef BUILD_ATARI7800

/**
 * @brief   Atari 7800 Graphics Routines for 5cardstud
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 * @verbose The shared MARIA engine (maria.s): 32x24 cells, each a tile and a
 *          palette select. The NES port's drawing on the tiles mkchr.py
 *          makes; palette 1 turns the felt black, which is the status bar
 *          and the highlights. The map is read back for what a cell shows.
 */

#include <stdbool.h>
#include <string.h>
#include "maria.h"
#include "tiles.h"
#include "vars.h"
#include "../platform-specific/graphics.h"

#define CORNER_TOP 0
#define CORNER_BOTTOM 18

#define STATUS_ROW (HEIGHT - 1)

/* MARIA colours, hue << 4 | luminance. A PAL console's hues sit one higher
   than an NTSC one's for the same colour. */
#define C_BLACK 0x00
#define C_WHITE 0x0F
#define C_FELT  0xC3
#define C_RED   0x34
#define PAL_HUE 0x10

#define FELT 0                  /* palette 0: white, felt, red */
#define DARK 1                  /* palette 1: white, black, red */

bool always_render_full_cards = 0;

unsigned int frameCount;
unsigned char palDirty;

void waitvsync(void)
{
    mt_sync();
    ++frameCount;
    palDirty = 0;
}

static void put(unsigned char x, unsigned char y, unsigned char tile)
{
    if (x >= WIDTH || y >= HEIGHT)
        return;
    mt_at(x, y);
    mt_put(tile);
}

static unsigned char peek(unsigned char x, unsigned char y)
{
    if (x >= WIDTH || y >= HEIGHT)
        return ' ';
    return mt_get(x, y);
}

/* n cells from (x,y) to palette p; the row's list is rebuilt next frame. */
static void setPalette(unsigned char x, unsigned char y, unsigned char n, unsigned char p)
{
    unsigned char *a = mt_attr + y * WIDTH + x;
    unsigned char i;

    if (y >= HEIGHT || x >= WIDTH)
        return;
    if (n > WIDTH - x)
        n = WIDTH - x;
    for (i = 0; i < n && a[i] == p; i++) ;
    if (i == n)
        return;
    mt_at(x, y);
    mt_setpal(p, n);
    palDirty = 1;
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

/* Up to n characters of s from (x,y). */
static void putText(unsigned char x, unsigned char y, const char *s, unsigned char n)
{
    unsigned char c;

    if (y >= HEIGHT || x >= WIDTH)
        return;
    mt_at(x, y);
    while (n-- && (c = (unsigned char) *s++) != 0)
        mt_put(textGlyph(c));
}

/**
 * @brief Initialize graphics mode; set palette.
 */
void initGraphics()
{
    unsigned char hue;

    mt_init(' ');
    hue = mt_pal ? PAL_HUE : 0;
    mt_background(C_BLACK);
    mt_palette(FELT, C_WHITE, C_FELT + hue, C_RED + hue);
    mt_palette(DARK, C_WHITE, C_BLACK, C_RED + hue);
    setPalette(0, STATUS_ROW, WIDTH, DARK);
    waitvsync();
}

void drawChip(unsigned char x, unsigned char y)
{
    put(x, y, T_CHIP_RG);
}

/* Let palette changes land before the game goes on to something slow. */
void drawBuffer()
{
    if (palDirty)
        waitvsync();
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned char i;
    // Correct coordinates;
    w++;
    h++;

    // Put box corners at coordinate extents
    put(x, y, T_BOX_TL_RG);
    put(x + w, y, T_BOX_TR_RG);
    put(x, y + h, T_BOX_BL_RG);
    put(x + w, y + h, T_BOX_BR_RG);

    // Horizontal rules
    for (i = 1; i < w; i++)
    {
        put(x + i, y, T_BOX_H_RG);
        put(x + i, y + h, T_BOX_H_RG);
    }

    // Vertical rules
    for (i = 1; i < h; i++)
    {
        put(x, y + i, T_BOX_V_RG);
        put(x + w, y + i, T_BOX_V_RG);
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
    putText(x, y, s, WIDTH);
}

/**
 * @brief Draw the 5 Card Stud Logo
 */
void drawLogo()
{
    unsigned char i = 4;
    drawText(WIDTH/2-5,++i, "           ");
    drawText(WIDTH/2-5,++i, " FUJI  NET ");
    drawText(WIDTH/2-5,++i, "           ");
    drawText(WIDTH/2-5,++i, "5 CARD STUD");
    drawText(WIDTH/2-5,++i, "           ");
}

/* Set while a two-line status message has taken row 22 for the bar. */
static bool tallStatus;

void clearStatusBar()
{
    // Row 23 only; row 22 is otherwise left alone, as the border aces extend
    // into it -- unless a two-line message took it, when it goes back to felt.
    mt_at(0, STATUS_ROW);
    mt_fill(' ', WIDTH);
    setPalette(0, STATUS_ROW, WIDTH, DARK);
    if (tallStatus)
    {
        tallStatus = false;
        mt_at(0, 22);
        mt_put(T_SCREEN_BL_GK);
        mt_fill(' ', WIDTH-2);
        mt_put(T_SCREEN_BR_GK);
        setPalette(0, 22, WIDTH, FELT);
    }
}

/**
 * @brief Clear the screen
 */
void resetScreen()
{
    mt_clear(' ');
    tallStatus = false;
    setPalette(0, STATUS_ROW, WIDTH, DARK);
    palDirty = 1;

    // Round the felt off against the border/status bar, like the MS-DOS build.
    put(0, 0, T_SCREEN_TL_GK);
    put(WIDTH-1, 0, T_SCREEN_TR_GK);
    put(0, 22, T_SCREEN_BL_GK);
    put(WIDTH-1, 22, T_SCREEN_BR_GK);
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
    unsigned char rank, suitTile, rankBase, i;

    if (x==WIDTH-3 && s[0]!='?' && peek(x,y+1)==T_BACK_L_TOP_RW) {
        drawCard(x+1,y,PARTIAL_RIGHT,"??",false);
    }

    if (partial == PARTIAL_LEFT)
    {
        put(x, y++, T_CARD_TL_RG);
        put(x, y++, T_BACK_L_TOP_RW);
        put(x, y++, T_BACK_L_MID_RW);
        put(x, y++, T_BACK_L_BOT_RW);
        put(x, y++, T_CARD_BL_RG);
    }
    else if (partial == PARTIAL_RIGHT)
    {
        x++;
        put(x, y++, T_CARD_TOP_TRIM_RG);
        put(x, y++, T_BACK_RCOL_TOP_RW);
        put(x, y++, T_BACK_RCOL_MID_RW);
        put(x, y++, T_BACK_RCOL_BOT_RW);
        put(x, y++, T_CARD_BOT_TRIM_RG);
    }
    else // FULL CARD
    {
        switch (s[1])
        {
        case 'h' :
            suitTile=T_SUIT_HEART_RW;
            rankBase=T_RANK_RW;
            break;
        case 'd' :
            suitTile=T_SUIT_DIAMOND_RW;
            rankBase=T_RANK_RW;
            break;
        case 'c' :
            suitTile=T_SUIT_CLUB_KW;
            rankBase=T_RANK_KW;
            break;
        case 's' :
            suitTile=T_SUIT_SPADE_KW;
            rankBase=T_RANK_KW;
            break;
        default:
            suitTile=T_CARD_VERT_RW;    // Something wrong.
            rankBase=T_RANK_RW;
            break;
        }

        // If card is overturned, draw the back
        if (s[0]=='?')
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
            put(x, y, T_CARD_TL_RG);
            put(x+1, y, T_CARD_TOP_RG);
            put(x+2, y, T_CARD_TR_STUB_RG);
            y++;
            put(x, y, T_BACK_L_TOP_RW);
            put(x+1, y, T_BACK_R_TOP_RW);
            put(x+2, y, T_CARD_VERT_RG);
            y++;
            put(x, y, T_BACK_L_MID_RW);
            put(x+1, y, T_BACK_R_MID_RW);
            put(x+2, y, T_CARD_VERT_RG);
            y++;
            put(x, y, T_BACK_L_BOT_RW);
            put(x+1, y, T_BACK_R_BOT_RW);
            put(x+2, y, T_CARD_VERT_RG);
            y++;
            put(x, y, T_CARD_BL_RG);
            put(x+1, y, T_CARD_BOT_RG);
            put(x+2, y, T_CARD_BR_STUB_RG);
        }
        else // Draw the full card.
        {
            // Top card border
            if (peek(x+1,y)!=T_CARD_TOP_RG)
            {
                put(x, y, T_CARD_TL_RG);
                put(x+1, y, T_CARD_TOP_RG);
            }
            if (peek(x+2,y)==' ')
                put(x+2, y, T_CARD_TR_STUB_RG);

            switch (s[0])
            {
            case 't':
                rank=8; // 10
                break;
            case 'j':
                rank=9;
                break;
            case 'q':
                rank=10;
                break;
            case 'k':
                rank=11;
                break;
            case 'a':
                rank=12;
                break;
            default:
                rank=s[0]-'2';
                break;
            }

            // Left border and card value
            y++;
            put(x, y, T_CARD_VERT_RW);
            put(x+1, y, rankBase + rank);
            if (peek(x+2,y)==' ')
                put(x+2, y, T_CARD_VERT_RG);

            // Interior (hole-card marker band if hidden)
            y++;
            if (isHidden)
            {
                put(x, y, T_HIDDEN_L_RW);
                put(x+1, y, T_HIDDEN_R_RW);
            }
            else
            {
                put(x, y, T_CARD_VERT_RW);
                put(x+1, y, T_BLANK_W);
            }
            if (peek(x+2,y)==' ')
                put(x+2, y, T_CARD_VERT_RG);

            // Suit
            y++;
            put(x, y, T_CARD_VERT_RW);
            put(x+1, y, suitTile);
            if (peek(x+2,y)==' ')
                put(x+2, y, T_CARD_VERT_RG);

            // Bottom border
            y++;
            if (peek(x+1,y)!=T_CARD_BOT_RG)
            {
                put(x, y, T_CARD_BL_RG);
                put(x+1, y, T_CARD_BOT_RG);
            }
            if (peek(x+2,y)==' ')
                put(x+2, y, T_CARD_BR_STUB_RG);
        }
    }
}

/* A message too long for one row (the round result) takes row 22 into the
   bar as well, word-wrapped across the two. */
void drawStatusTextAt(unsigned char x, const char* s)
{
    unsigned char brk;

    if (strlen(s) <= WIDTH)
    {
        putText(x, STATUS_ROW, s, WIDTH);
        return;
    }

    for (brk = WIDTH; brk > 0 && s[brk] != ' '; brk--) ;
    if (brk == 0)
        brk = WIDTH;

    tallStatus = true;
    mt_at(0, 22);
    mt_fill(' ', WIDTH);
    setPalette(0, 22, WIDTH, DARK);
    mt_at(0, STATUS_ROW);
    mt_fill(' ', WIDTH);
    putText(0, 22, s, brk);
    putText(0, STATUS_ROW, s + brk + (s[brk] == ' '), WIDTH);
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

/* On the status row a highlight is the move under the cursor, shown on felt
   instead of black. Elsewhere the line is under a name, and the name is
   shown on black instead of felt. */
static void line(unsigned char x, unsigned char y, unsigned char w, bool on)
{
    if (y >= HEIGHT || w == 0)
        return;
    if (y == STATUS_ROW)
        setPalette(x, y, w, on ? FELT : DARK);
    else if (y > 0)
        setPalette(x, y - 1, w, on ? DARK : FELT);
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

#endif /* BUILD_ATARI7800 */
