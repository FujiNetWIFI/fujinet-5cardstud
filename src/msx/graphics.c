/**
 * @brief   MSX Graphics Routines for 5cardstud
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#ifdef BUILD_MSX

#include <stdbool.h>
#include <video/tms99x8.h>
#include <conio.h>
#include <sys/ioctl.h>
#include "udg.h"

bool always_render_full_cards = 0;

/**
 * @brief Initialize graphics mode; set palette.
 */
void initGraphics()
{
    void *param = &udg;
    vdp_set_mode(2);
    console_ioctl(IOCTL_GENCON_SET_UDGS,&param);
    vdp_color(VDP_INK_BLACK,VDP_INK_DARK_GREEN,VDP_INK_DARK_GREEN);
    clrscr();
}

void drawChip(unsigned char x, unsigned char y)
{
}

void drawBuffer()
{
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
}

void drawText(unsigned char x, unsigned char y, const char* s)
{
}

void clearStatusBar()
{
}

void resetScreen()
{
}

void hideLine(unsigned char x, unsigned char y, unsigned char w)
{
}

void disableDoubleBuffer()
{
}

void enableDoubleBuffer()
{
}

void drawCard(unsigned char x, unsigned char y, unsigned char partial, const char* s, bool isHidden)
{
}

void drawStatusText(const char* s)
{
}

void drawStatusTextAt(unsigned char x, const char* s)
{
}

void drawLogo()
{
}

unsigned char cycleNextColor()
{
}

void drawStatusTimer()
{
}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
}

void setColorMode(unsigned char mode)
{
}

void drawBorder()
{
}

#endif /* BUILD_MSX */
