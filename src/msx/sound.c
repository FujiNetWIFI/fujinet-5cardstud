#ifdef BUILD_MSX

/**
 * @brief   MSX Sound Routines
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#include <stdint.h>
#include <psg.h>
#include <sound.h>

void waitvsync(void);

/**
 * @brief Brain dead beep routine
 * @param hz Frequency in Hz
 * @param gate Delay in vertical blanks
 * @param postGate Delay after tone off in vertical blanks
 */
void beep(int hz, int gate, int postGate)
{
    psg_volume(1,10);
    psg_tone(1,psgT(hz));
    while (gate--)
        waitvsync();
    psg_volume(1,0);
    while (postGate--)
        waitvsync();
}

void soundDealCard()
{
    beep(150,1,5);
}

void soundJoinGame()
{
    beep(430,5,8);
    beep(340,5,0);
    beep(500,5,0);
}

void soundPlayerLeft()
{
    uint8_t i;
    for (i=80;i>=50;i-=10)
        beep(i,2,15);
}

void soundPlayerJoin()
{
    uint8_t i;
    for (i=50;i<=80;i+=10)
        beep(i,2,15);
}

void soundGameDone()
{
    beep(311,10,0);
    beep(330,20,0);
    beep(392,10,0);
    beep(415,20,0);
}

void soundMyTurn()
{
    beep(430,4,2);
    beep(430,4,2);
}

void soundTick()
{
    beep(50,2,0);
}

void soundCursor()
{
    beep(300,2,0);
}

void soundCursorInvalid()
{
    beep(100,2,0);
}

void soundSelectMove()
{
    beep(300,3,1);
    beep(350,3,0);
}

void initSound()
{
    psg_init();
    psg_channels(chanAll,chanNone);
}

void soundTakeChip(uint16_t counter)
{
    beep(50+counter*20,2,2);
}

#endif /* BUILD_MSX */
