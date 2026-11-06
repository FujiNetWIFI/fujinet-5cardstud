#ifdef __H89__

/**
 * @brief   Sound - Heathkit H89
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * The only sound an H89 makes is the H19's bell, so it marks the moments
 * that need the player's attention -- sitting down, their turn, a hand's
 * result -- and the rest stay quiet.
 */

#include <stdint.h>
#include "console.h"

void initSound() {}

void soundJoinGame()
{
    termBell();
}

void soundMyTurn()
{
    termBell();
}

void soundGameDone()
{
    termBell();
}

void soundDealCard() {}
void soundTick() {}
void soundPlayerJoin() {}
void soundPlayerLeft() {}
void soundSelectMove() {}
void soundCursor() {}
void soundCursorInvalid() {}
void soundTakeChip(uint16_t counter) { (void)counter; }

#endif /* __H89__ */
