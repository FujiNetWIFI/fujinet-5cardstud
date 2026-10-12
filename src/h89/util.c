#ifdef __H89__

/**
 * @brief   Utility Functions - Heathkit H89
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * The H89's clock interrupts every 2 ms, and both HDOS and the CP/M BIOS
 * count the ticks in a word (console.h's TICKS). The shared code counts in
 * 60ths of a second, so a frame here is 25/3 ticks: 8, 8, then 9.
 */

#include <stdint.h>
#include <stdlib.h>
#include "console.h"
#include "../platform-specific/graphics.h"
#include "../platform-specific/util.h"

static uint16_t lastFrame, timerBase;
static uint8_t framePhase;

void waitvsync(void)
{
    static uint8_t len;

    // At least a frame since the last one returned. Drawing over the 9600
    // baud console can take longer than that; the game just runs at the
    // pace of the terminal then.
    len = (framePhase == 2) ? 9 : 8;
    while ((uint16_t)(TICKS - lastFrame) < len)
        ;
    lastFrame = TICKS;
    framePhase = (framePhase == 2) ? 0 : framePhase + 1;
}

void resetTimer(void)
{
    timerBase = TICKS;
}

int getTime(void)
{
    // 60ths of a second since resetTimer(); the tick word wraps after 131 s,
    // longer than any move clock
    return (int)(((uint32_t)(uint16_t)(TICKS - timerBase) * 3) / 25);
}

void quit(void)
{
    // There is no H89 lobby to load: back to HDOS or CP/M, with the
    // terminal as it was
    resetGraphics();
    exit(0);
}

#endif /* __H89__ */
