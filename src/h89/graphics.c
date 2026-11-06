#ifdef __H89__

/**
 * @brief   Graphics - Heathkit H89 (H19 terminal, 80x24 + 25th line)
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * Everything is drawn with the H19's own characters: its graphics set (line
 * drawing, diagonals, triangles, quadrant blocks, the checkerboard, the
 * bullet) and reverse video. The ROM has no card suits, so each suit is a
 * 2x2 picture made of triangles and quadrants:
 *
 *   spades ◢◣   hearts ▙▟   diamonds ◢◣   clubs ▗▖
 *          ▝▘          ◥◤            ◥◤         ●●
 *
 * Cards are 5x5 -- rank top left, suit below -- and backs are checkerboard.
 * A hand is a fan: the shared code lays a seat's cards every 4 columns
 * (src/h89/vars.h's CARD_SPREAD), so each card's right border is the next
 * one's left, and where two meet the border column turns into T junctions.
 * An opponent's face-down card is covered but for a two column sliver; your
 * own has a checkerboard strip down its left to show the others can't see it.
 * The table is an oval rail of diagonals and edge lines between the seats'
 * hands (resetScreen), the other screens a frame of the same with an ace in
 * each corner (drawBorder). Chips are bullets. The status bar is the 25th
 * line, where the selected move is shown in reverse video; drawLine/hideLine,
 * which are underlines elsewhere, reverse the text above them here (the
 * active player's name, the table list's header). The H19 is monochrome, so
 * the colour toggle draws hearts and diamonds in reverse video instead of red.
 *
 * Drawing goes into console.c's back buffer and flush() sends only what
 * changed. Unbuffered (disableDoubleBuffer), each call flushes when it is
 * done, so dealing and flipping show card by card.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "console.h"
#include "logo.h"
#include "../platform-specific/graphics.h"

#ifndef CARD_W
#define CARD_W 5
#endif
#define CARD_H 5

// The table rail: an oval from row RAIL_TOP to RAIL_BOTTOM, column RAIL_LEFT
// to RAIL_RIGHT, its corners cut by two diagonal steps. The side seats'
// hands end at columns 20 and 59 (src/h89/vars.c).
#define RAIL_TOP 8
#define RAIL_BOTTOM 16
#define RAIL_LEFT 22
#define RAIL_RIGHT 57

bool always_render_full_cards = 0;
unsigned char colorMode = 0;

static bool buffered;

// Suit pictures, upper left, upper right, lower left, lower right
static const uint8_t suitSpade[4] = {REV | G_TRI_UL, REV | G_TRI_UR, G_QUAD_UR, G_QUAD_UL};
static const uint8_t suitHeart[4] = {REV | G_QUAD_UR, REV | G_QUAD_UL, G_TRI_UR, G_TRI_UL};
static const uint8_t suitDiamond[4] = {REV | G_TRI_UL, REV | G_TRI_UR, G_TRI_UR, G_TRI_UL};
static const uint8_t suitClub[4] = {G_QUAD_LR, G_QUAD_LL, G_BULLET, G_BULLET};

// XOR'd into the face of the card being drawn: REV for a "red" card. Not
// the border, which a fanned card shares with its neighbour.
static uint8_t cardRev;

/// @brief Unbuffered, send what this call drew
static void done(void)
{
    if (!buffered)
        flush();
}

static void blankTable(void)
{
    blankRows(0, STATUS_ROW);
}

void enableDoubleBuffer()
{
    buffered = true;
}

void disableDoubleBuffer()
{
    flush();
    buffered = false;
}

void drawBuffer()
{
    flush();
}

// Call to clear the screen to an empty table. Never flushed here: it is
// always followed by drawBorder() or a full table.
void resetScreen()
{
    static uint8_t y;

    blankTable();

    // The rail
    put(RAIL_LEFT + 1, RAIL_TOP, G_SLASH);
    fill(RAIL_LEFT + 2, RAIL_TOP, RAIL_RIGHT - RAIL_LEFT - 3, G_BAR_TOP);
    put(RAIL_RIGHT - 1, RAIL_TOP, G_BACKSLASH);
    put(RAIL_LEFT, RAIL_TOP + 1, G_SLASH);
    put(RAIL_RIGHT, RAIL_TOP + 1, G_BACKSLASH);
    for (y = RAIL_TOP + 2; y < RAIL_BOTTOM - 1; y++)
    {
        put(RAIL_LEFT, y, G_BAR_LEFT);
        put(RAIL_RIGHT, y, G_BAR_RIGHT);
    }
    put(RAIL_LEFT, RAIL_BOTTOM - 1, G_BACKSLASH);
    put(RAIL_RIGHT, RAIL_BOTTOM - 1, G_SLASH);
    put(RAIL_LEFT + 1, RAIL_BOTTOM, G_BACKSLASH);
    fill(RAIL_LEFT + 2, RAIL_BOTTOM, RAIL_RIGHT - RAIL_LEFT - 3, G_BAR_BOTTOM);
    put(RAIL_RIGHT - 1, RAIL_BOTTOM, G_SLASH);

    // A rule above the status line
    fill(0, STATUS_ROW - 1, SCR_W, G_BAR_BOTTOM);
}

void clearStatusBar()
{
    fill(0, STATUS_ROW, SCR_W, ' ');
    done();
}

void drawStatusTextAt(unsigned char x, const char* s)
{
    static uint8_t c;

    // Keep the reverse video the move cursor laid on these cells
    while ((c = (uint8_t)*s++) && x < SCR_W)
    {
        if (c >= 0x20 && c < 0x7F)
        {
            put(x, STATUS_ROW, c | (peek(x, STATUS_ROW) & REV));
            x++;
        }
    }
    done();
}

void drawStatusText(const char* s)
{
    fill(0, STATUS_ROW, SCR_W, ' ');
    drawStatusTextAt(0, s);
}

void drawStatusTimer()
{
    // An hourglass after the countdown
    put(SCR_W - 1, STATUS_ROW, REV | G_X);
    done();
}

void drawText(unsigned char x, unsigned char y, const char* s)
{
    static uint8_t c;

    while ((c = (uint8_t)*s++) && x < SCR_W)
    {
        if (c >= 0x20 && c < 0x7F)
            put(x++, y, c);
    }
    done();
}

static void cardCell(uint8_t x, uint8_t y, uint8_t code)
{
    put(x, y, code ^ cardRev);
}

/// @brief Is the cell at x,y on a card's top edge, i.e. is there a card
/// beside the border column being drawn?
static bool cardTop(uint8_t x, uint8_t y)
{
    static uint8_t c;

    c = peek(x, y) & 0x7F;
    return c == G_HLINE || c == G_TEE_DOWN || (c >= G_CORNER_UR && c <= G_CORNER_UL);
}

/// @brief A card's left or right border: its own corners, or T junctions
/// where it meets the card beside it
static void cardEdge(uint8_t x, uint8_t y, bool shared, uint8_t top, uint8_t bottom)
{
    static uint8_t r;

    put(x, y, shared ? G_TEE_DOWN : top);
    for (r = 1; r < CARD_H - 1; r++)
        put(x, y + r, G_VLINE);
    put(x, y + CARD_H - 1, shared ? G_TEE_UP : bottom);
}

void drawCard(unsigned char x, unsigned char y, unsigned char partial, const char* s, unsigned char isHidden)
{
    static const uint8_t *suit;
    static bool leftShared, rightShared;
    static uint8_t r;

    // The table is repainted in full each time and the shared code deals a
    // seat's cards in order, so a card is always drawn whole and the next
    // one covers it: a PARTIAL face-down card is a whole back with the next
    // card two columns along.
    (void)partial;

    cardRev = 0;
    switch (s[1])
    {
    case 's':
        suit = suitSpade;
        break;
    case 'h':
        suit = suitHeart;
        cardRev = colorMode ? REV : 0;
        break;
    case 'd':
        suit = suitDiamond;
        cardRev = colorMode ? REV : 0;
        break;
    case 'c':
        suit = suitClub;
        break;
    default:
        suit = 0; // face down
        break;
    }

    // Look for the neighbours before drawing over anything: the card before
    // it in a left-to-right fan, the one after it in a right-to-left fan
    leftShared = cardTop(x - 1, y);
    rightShared = cardTop(x + CARD_W, y);

    // Border
    cardEdge(x, y, leftShared, G_CORNER_UL, G_CORNER_LL);
    cardEdge(x + CARD_W - 1, y, rightShared, G_CORNER_UR, G_CORNER_LR);
    fill(x + 1, y, CARD_W - 2, G_HLINE);
    fill(x + 1, y + CARD_H - 1, CARD_W - 2, G_HLINE);

    if (!suit)
    {
        for (r = 1; r < CARD_H - 1; r++)
            fill(x + 1, y + r, CARD_W - 2, G_CHECKER);
        done();
        return;
    }

    // Rank, top left: 't' is 10
    cardCell(x + 2, y + 1, ' ');
    cardCell(x + 3, y + 1, ' ');
    switch (s[0])
    {
    case 't':
        cardCell(x + 1, y + 1, '1');
        cardCell(x + 2, y + 1, '0');
        break;
    case 'j':
    case 'q':
    case 'k':
    case 'a':
        cardCell(x + 1, y + 1, s[0] - 'a' + 'A');
        break;
    default:
        cardCell(x + 1, y + 1, s[0]);
        break;
    }

    // Suit, bottom right; down the left, a strip marking your face-down
    // card
    for (r = 2; r < CARD_H - 1; r++)
        cardCell(x + 1, y + r, isHidden ? G_CHECKER : ' ');
    cardCell(x + 2, y + 2, suit[0]);
    cardCell(x + 3, y + 2, suit[1]);
    cardCell(x + 2, y + 3, suit[2]);
    cardCell(x + 3, y + 3, suit[3]);

    done();
}

void drawChip(unsigned char x, unsigned char y)
{
    put(x, y, G_BULLET);
    done();
}

void drawBlank(unsigned char x, unsigned char y)
{
    put(x, y, ' ');
    done();
}

/// @brief Reverse (or not) the w cells above row y, or row y itself on the
/// status line: where the other platforms underline, the H19 reverses
static void reverseRun(uint8_t x, uint8_t y, uint8_t w, bool on)
{
    static uint8_t c;

    if (y != STATUS_ROW)
    {
        if (!y)
            return;
        y--;
    }

    while (w-- && x < SCR_W)
    {
        c = peek(x, y);
        put(x, y, on ? (c | REV) : (c & ~REV));
        x++;
    }
    done();
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    reverseRun(x, y, w, true);
}

void hideLine(unsigned char x, unsigned char y, unsigned char w)
{
    reverseRun(x, y, w, false);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    static uint8_t r;

    put(x, y, G_CORNER_UL);
    fill(x + 1, y, w, G_HLINE);
    put(x + w + 1, y, G_CORNER_UR);
    for (r = 1; r <= h; r++)
    {
        put(x, y + r, G_VLINE);
        fill(x + 1, y + r, w, ' ');
        put(x + w + 1, y + r, G_VLINE);
    }
    put(x, y + h + 1, G_CORNER_LL);
    fill(x + 1, y + h + 1, w, G_HLINE);
    put(x + w + 1, y + h + 1, G_CORNER_LR);
    done();
}

void drawBorder()
{
    static uint8_t r;
    static bool wasBuffered;

    // Whatever resetScreen() drew goes: the frame replaces the table. Batch
    // the lot so an unbuffered caller never sees the rail go out and back.
    wasBuffered = buffered;
    buffered = true;

    blankTable();

    // A frame round the screen, its corners cut by diagonals like the rail's
    put(0, 0, G_SLASH);
    fill(1, 0, SCR_W - 2, G_BAR_TOP);
    put(SCR_W - 1, 0, G_BACKSLASH);
    for (r = 1; r < STATUS_ROW - 1; r++)
    {
        put(0, r, G_BAR_LEFT);
        put(SCR_W - 1, r, G_BAR_RIGHT);
    }
    put(0, STATUS_ROW - 1, G_BACKSLASH);
    fill(1, STATUS_ROW - 1, SCR_W - 2, G_BAR_BOTTOM);
    put(SCR_W - 1, STATUS_ROW - 1, G_SLASH);

    // An ace of each suit in the corners
    drawCard(2, 1, FULL_CARD, "as", 0);
    drawCard(SCR_W - 2 - CARD_W, 1, FULL_CARD, "ah", 0);
    drawCard(2, STATUS_ROW - 1 - CARD_H, FULL_CARD, "ad", 0);
    drawCard(SCR_W - 2 - CARD_W, STATUS_ROW - 1 - CARD_H, FULL_CARD, "ac", 0);

    buffered = wasBuffered;
    done();
}

void drawLogo()
{
    static const uint8_t *p;
    static uint8_t ly, lx, n;

    // 5 CARD / STUD in quadrant-block letters (src/h89/mklogo.py)
    p = logo;
    while (*p != 0xFF)
    {
        ly = *p++;
        lx = *p++;
        n = *p++;
        while (n--)
            put(lx++, ly, *p++);
    }

    // and a rule with the maker's name under it
    fill(WIDTH / 2 - 17, 10, 10, G_HLINE);
    drawText(WIDTH / 2 - 6, 10, "F U J I N E T");
    fill(WIDTH / 2 + 8, 10, 10, G_HLINE);
    done();
}

void initGraphics()
{
    termInit();
}

void resetGraphics()
{
    termExit();
}

unsigned char cycleNextColor()
{
    colorMode = !colorMode;
    return colorMode;
}

void setColorMode(unsigned char mode)
{
    colorMode = mode ? 1 : 0;
}

#endif /* __H89__ */
