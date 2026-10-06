#ifdef BUILD_SMS

/**
 * @brief   Sega Master System on-screen keyboard
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#include <string.h>
#include "joystick.h"
#include "vars.h"
#include "../platform-specific/graphics.h"
#include "../platform-specific/sound.h"
#include "../platform-specific/input.h"
// After misc.h (via graphics.h): conio.h's gotoxy has to be the one defined.
#include <arch/sms.h>

/*
  The ColecoVision and NES clients' grid (src/coleco/osk.c): the cursor
  position IS the character, so there is no shift key and no paging. Cancel is
  the OK cell's absence -- the caller only reaches this screen when a name is
  required -- and the grid never runs a mailbox transaction. Uppercase only:
  drawText() uppercases everything it prints.

  Eight keys wide rather than ten, so that it fits between the border aces;
  the key under the cursor is a gold key cap.

    d-pad  move      1  take the key      2  backspace      Pause  OK
*/

#define OSK_COLS   8
#define OSK_ROWS   5
#define OSK_PITCH  3
#define OSK_X      4
#define OSK_Y      17

/* The last row is one short: its eighth key does not exist. */
#define OSK_LAST_ROW_COLS 7

#define CELL_SPACE (OSK_COLS * 4 + 4)   /* index 36 */
#define CELL_DEL   (CELL_SPACE + 1)
#define CELL_OK    (CELL_SPACE + 2)

static const char cells[] =
  "ABCDEFGH"
  "IJKLMNOP"
  "QRSTUVWX"
  "YZ012345"
  "6789";

static const char *labelSpace = "SP ";
static const char *labelDel   = "<- ";
static const char *labelOk    = "OK ";

void drawHiliteText(unsigned char x, unsigned char y, const char* s);  /* graphics.c */

#define REPEAT_FIRST 18                 /* vblanks before a held direction repeats */
#define REPEAT_NEXT  5

static unsigned char curX, curY;

static unsigned char cellCols(unsigned char row)
{
  return row == OSK_ROWS - 1 ? OSK_LAST_ROW_COLS : OSK_COLS;
}

static void drawCell(unsigned char col, unsigned char row, unsigned char marked)
{
  unsigned char idx = row * OSK_COLS + col;
  const char *label;
  char glyph[4];

  if (idx == CELL_SPACE)
    label = labelSpace;
  else if (idx == CELL_DEL)
    label = labelDel;
  else if (idx == CELL_OK)
    label = labelOk;
  else
  {
    glyph[0] = ' ';
    glyph[1] = cells[idx];
    glyph[2] = ' ';
    glyph[3] = 0;
    label = glyph;
  }

  if (marked)
    drawHiliteText(OSK_X + col * OSK_PITCH, OSK_Y + row, label);
  else
    drawText(OSK_X + col * OSK_PITCH, OSK_Y + row, label);
}

static void drawGrid(void)
{
  unsigned char row, col;

  for (row = 0; row < OSK_ROWS; row++)
    for (col = 0; col < cellCols(row); col++)
      drawCell(col, row, row == curY && col == curX);
}

static void clearGrid(void)
{
  unsigned char row;

  for (row = 0; row < OSK_ROWS; row++)
    drawText(OSK_X, OSK_Y + row, "                        ");
}

/* Repaint the field and its trailing cursor chip, the same shape
   inputFieldCycle() draws on the platforms that have a keyboard. */
static void drawField(unsigned char x, unsigned char y, unsigned char max,
                      const char *buffer)
{
  unsigned char len = (unsigned char) strlen(buffer);

  drawText(x, y, "         ");
  drawText(x, y, buffer);
  if (len < max)
    drawChip(x + len, y);
}

static void appendChar(char *buffer, unsigned char *len, unsigned char max,
                       char ch)
{
  if (*len >= max)
  {
    soundCursorInvalid();
    return;
  }
  buffer[(*len)++] = ch;
  buffer[*len] = 0;
  soundCursor();
}

void platformNameEntry(unsigned char x, unsigned char y, unsigned char max,
                       char *buffer)
{
  unsigned char len = (unsigned char) strlen(buffer);
  unsigned char hold = 0, lastDir = 0, lastButtons = 0, lastPause = pause_flag;
  unsigned char pad, dir, move, buttons, pressed, idx;

  curX = 0;
  curY = 0;
  drawGrid();
  drawField(x, y, max, buffer);

  for (;;)
  {
    waitvsync();

    pad = (unsigned char) ~IO_DC;
    dir = pad & JOY_DIRS_MASK;
    buttons = pad & (JOY_BTN_1_MASK | JOY_BTN_2_MASK);

    /* Auto-repeat is counted in vblanks: this loop runs once per frame. */
    move = 0;
    if (dir != lastDir)
    {
      lastDir = dir;
      hold = REPEAT_FIRST;
      move = dir;
    }
    else if (dir && --hold == 0)
    {
      hold = REPEAT_NEXT;
      move = dir;
    }

    if (move)
    {
      drawCell(curX, curY, 0);

      if ((move & JOY_LEFT_MASK) && curX)
        curX--;
      else if ((move & JOY_RIGHT_MASK) && curX + 1 < cellCols(curY))
        curX++;
      else if ((move & JOY_UP_MASK) && curY)
        curY--;
      else if ((move & JOY_DOWN_MASK) && curY + 1 < OSK_ROWS)
        curY++;

      if (curX >= cellCols(curY))
        curX = cellCols(curY) - 1;

      drawCell(curX, curY, 1);
      soundCursor();
    }

    pressed = buttons & ~lastButtons;
    lastButtons = buttons;

    /* 2 and Pause are shortcuts over the grid; 1 takes whatever the cursor
       is sitting on. */
    idx = curY * OSK_COLS + curX;
    if (pause_flag != lastPause)
    {
      lastPause = pause_flag;
      idx = CELL_OK;
    }
    else if (pressed & JOY_BTN_2_MASK)
      idx = CELL_DEL;
    else if (!(pressed & JOY_BTN_1_MASK))
      continue;

    if (idx == CELL_OK)
    {
      if (!len)
      {
        soundCursorInvalid();
        continue;
      }
      break;
    }

    if (idx == CELL_DEL)
    {
      if (!len)
      {
        soundCursorInvalid();
        continue;
      }
      buffer[--len] = 0;
      soundCursor();
    }
    else
      appendChar(buffer, &len, max, idx == CELL_SPACE ? ' ' : cells[idx]);

    drawField(x, y, max, buffer);
  }

  clearGrid();

  // Forget the presses typed here, Pause included, before the game reads keys.
  initPlatformKeyboardInput();
}

#endif /* BUILD_SMS */
