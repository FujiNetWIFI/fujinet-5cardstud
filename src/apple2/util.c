#ifdef __APPLE2__

#include<peekpoke.h>
#include<stdlib.h>
#include<stdint.h>
#include <fujinet-fuji.h>
/*
 Screen related variables
*/


// Screen specific player/bet coordinates
unsigned char playerXMaster[] = { 17,1, 1, 1, 15, 37,37, 37 };
unsigned char playerYMaster[] = { 18, 17, 10, 3, 3,3,10,17 };

char playerDirMaster[] = { 1,1,1,1,1,-1,-1,-1 };
char playerBetXMaster[] = { 1,10,10,10,3,-8,-8,-8 };
char playerBetYMaster[] = { -2, -2, 1,4,4,4,1,-2 };

// Simple hard coded arrangement of players around the table based on player count.
// These refer to index positions in the Master arrays above
// Downside is new players will cause existing player positions to move.

//                               2                3                4  
char playerCountIndex[] = {0,4,0,0,0,0,0,0, 0,2,6,0,0,0,0,0, 0,2,4,6,0,0,0,0, 
// 5                6                 7                8
   0,2,3,5,6,0,0,0, 0,2,3,4,5,6,0,0,  0,2,3,4,5,6,7,0, 0,1,2,3,4,5,6,7};



void resetGraphics();
uint16_t jiffieTimer;

void resetTimer() {
  jiffieTimer=0;
}

int getTime() {
  jiffieTimer+=3;
  return jiffieTimer;
}

void quit() {
  resetGraphics();
  fuji_set_boot_mode(FUJI_BOOT_MODE_LOBBY);
  // ROM in, then spoil the reset vector's checksum so the reset is a cold boot
  __asm__ ("bit $c082");
  __asm__ ("dec $03f4");
  __asm__ ("jmp ($fffc)");
}

#endif /* __APPLE2__ */