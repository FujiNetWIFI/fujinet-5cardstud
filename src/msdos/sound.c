#include <dos.h>
#include <stdint.h>

// PC Speaker ports and values
#define PIT_CONTROL_PORT 0x43
#define PIT_CHANNEL2_PORT 0x42
#define SPEAKER_CONTROL_PORT 0x61

// PIT frequency
#define PIT_FREQUENCY 1193180UL

/* Because I don't want to drag all of conio into this project! */
_WCIRTLINK extern unsigned inp(unsigned __port);
_WCIRTLINK extern unsigned outp(unsigned __port, unsigned __value);

extern unsigned int pitTicksPerMs;
extern unsigned int pitRead(void);
extern void calibratePit(void);

/* Absolute, unlike waitvsync(), so a 1-frame beep is still heard. One frame
 * per pass, so counter 0 cannot wrap mid wait. */
static void wait_frames(unsigned int frames)
{
    unsigned int start, want;

    if (!pitTicksPerMs)
        calibratePit();

    want = (unsigned int)(((unsigned long)pitTicksPerMs * 50UL) / 3UL);  /* 16.67ms */

    while (frames--) {
        start = pitRead();
        while ((unsigned int)(start - pitRead()) < want)
            ;                                /* PIT counts down */
    }
}

/**
 * @brief Beep the speaker for the specified # of 16.67ms frames
 * @param frequency Frequency in Hz
 * @param frames # of frames to sound
 * @param wait # of frames to wait after.
 */
void beep(unsigned int frequency, unsigned int frames, unsigned int wait) {
    unsigned int divisor;
    unsigned char tmp;

    // Calculate the divisor for the given frequency
    divisor = PIT_FREQUENCY / frequency;

    // Set the PIT to mode 3 (square wave) on channel 2
    outp(PIT_CONTROL_PORT, 0xB6); // 1011 0110

    // Send frequency divisor to channel 2 (low byte, then high byte)
    outp(PIT_CHANNEL2_PORT, divisor & 0xFF);        // Low byte
    outp(PIT_CHANNEL2_PORT, (divisor >> 8) & 0xFF); // High byte

    // Turn on the speaker (enable bit 0 and 1)
    tmp = inp(SPEAKER_CONTROL_PORT);
    outp(SPEAKER_CONTROL_PORT, tmp | 0x03);

    // Delay for # of frames
    wait_frames(frames);

    // Turn off the speaker (clear bit 0, keep bit 1 for PIT gate)
    tmp = inp(SPEAKER_CONTROL_PORT);
    outp(SPEAKER_CONTROL_PORT, tmp & ~0x03);

    // Wait for # of frames
    wait_frames(wait);
}

void initSound()
{
    // Not really used.
}

/**
 * WIP :)
 */
void soundJoinGame()
{
   beep(430,5,8);
   beep(340,5,0);
   beep(500,5,0);
}

void soundMyTurn()
{
  beep(430,4,2);
  beep(430,4,2);
}

void soundGameDone()
{
    beep(311,10,0);
    beep(330,20,0);
    beep(392,10,0);
    beep(415,20,0);
}

void soundDealCard()
{
    beep(150,1,5);
}

void soundTick()
{
    beep(50,2,0);
}

void soundPlayerJoin()
{
    uint8_t i;
    for (i=50;i<=80;i+=10)
        beep(i,2,15);
}

void soundPlayerLeft()
{
    uint8_t i;
    for (i=80;i>=50;i-=10)
        beep(i,2,15);
}

void soundSelectMove()
{
    beep(300,3,1);
    beep(350,3,0);
}

void soundCursor()
{
    beep(300,2,0);
}

void soundCursorInvalid()
{
    beep(100,2,0);
}

void soundTakeChip(uint16_t counter)
{
    beep(50+counter*20,2,2);
}
