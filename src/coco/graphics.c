#ifdef _CMOC_VERSION_

/*
  Graphics functionality
*/
#include "coco_bool.h"

#include "hires.h"
#include "vars.h"
#include "../platform-specific/graphics.h"
#include "../misc.h"

#define BOTTOM 175

#define USER_HAND_ROW       149
#define DISPLAYED_CARD_COL  14
#define DISPLAYED_CARD_ROW  48

#ifdef COCO3
#define CORNER_TOP 0
#define CORNER_BOTTOM 140
/* Status text sits directly on the felt like the Atari port; no bar. */
#define STATUS_BLANK 0
#define COCO3_ADJUST_Y y++;
#else
#define CORNER_TOP -5
#define CORNER_BOTTOM 144
#define STATUS_BLANK 0
#define COCO3_ADJUST_Y
#endif
#define CARDS_PER_ROW  15

#define OVERLAY_HEIGHT 30  /* A card on top of another one must be drawn this many pixels lower, so enough of the bottom card shows. */

#define STATUS_COL 6
#define STATUS_ROW 1  // in characters

#define DECK_COL 21  // in bytes of pixels
#define DECK_ROW 8  // in pixels

#define EVENT_ROW 13

#define REQUESTED_SUIT_COL 19  // in characters
#define REQUESTED_SUIT_ROW  9  // in characters

#define QUIT_GAME 0
#define NEXT_PLAYER_PLAYS 1
#define NEXT_PLAYER_PASSES 2
#define PLAY_NEW_GAME 3

#define RED 0b01010101
#define RED_RIGHT 0b01010100

extern uint8_t charset[];
#ifdef COCO3
uint8_t paletteBackup[16];
#else
uint8_t paletteBackup[8];
#endif

bool always_render_full_cards = 0;
uint8_t xor_mask=0;
uint8_t font_shift=0;
#ifdef COCO3
/* When set, hires_Draw recolours card-back glyphs via card_back_map.
   CARD_BACK() is a no-op on CoCo 1/2, which has no extended palette. */
uint8_t card_back = 0;
#define CARD_BACK(on) card_back = (on)
#else
#define CARD_BACK(on)
#endif

// Currently this is only for CoCo3
unsigned char colorMode=0;

void updateColors() {
    if (colorMode==1)
        setRGB();
    else
        setComposite();
}

unsigned char cycleNextColor() {
    ++colorMode;
    if (colorMode>2)
        colorMode=1;
    updateColors();
    return colorMode;
}

void setColorMode(unsigned char mode) {
    colorMode = mode;
}

void enableDoubleBuffer() {
    // Not implemented
}

void disableDoubleBuffer() {
    // not implemented
}

void drawTextAt(unsigned char x, unsigned char y, const char *s) {
    static unsigned char c;
#ifdef COCO3
    if (y<180)
        y++;
#endif
    while(*s) {
        c=*s++;
        if (c>=97) c-=32;
        hires_putc(x++,y,0,c);
    }
}

void clearStatusBar() {
  hires_Mask(0,179,WIDTH,13,STATUS_BLANK);
}

void drawBuffer() {
  waitvsync();
}

void drawStatusTextAt(unsigned char x, const char* s) {
  font_shift=1;
  drawTextAt(x, BOTTOM+6, s);
  font_shift=0;
}

void drawStatusText(const char* s) {
  static char* comma;
  
  unsigned char len = strlen(s);
  if (len>WIDTH) {
      comma = (char *)s;
    while (*comma++!=',');
    comma[0]=0;
    comma++;
    clearStatusBar();
    font_shift=1;
    drawTextAt(0, BOTTOM-1, s);
    drawTextAt(0, BOTTOM+8, comma);
    font_shift=0;
  } else {
    drawStatusTextAt(0, s);
    if (len<WIDTH)
      hires_Mask(len,179,WIDTH-len,13,STATUS_BLANK);
  }

}

void drawStatusTimer() {
#ifdef COCO3
  font_shift=1;
#endif
  
  hires_putc(WIDTH-1,BOTTOM+5,0, 0x28);
#ifdef COCO3
  font_shift=0;
#endif
}


void drawText(unsigned char x, unsigned char y, const char* s) {
  drawTextAt(x,y*8, s);
}


void drawChip(unsigned char x, unsigned char y) {
#ifdef COCO3
    hires_putc(x,y*8+1,0, 0x22);
#else
    hires_putc(x,y*8,0, 0x22);
#endif
  
}


// Call to clear the screen to an empty table
void resetScreen() {
    uint8_t i;

#ifdef COCO3
    /* Clear all 192 game scanlines via Task 1; the wavy table-edge
       decorations from the old 2bpp build are handled by drawBorder()
       later in Phase 2e. */
    hires_Mask(0, 0, WIDTH, HEIGHT * 8, 0);
#else
    memset((uint8_t *)SCREEN, 0, WIDTH * 179);
#endif
}

void drawCardAt(unsigned char x, unsigned char y, unsigned char partial, const char* s, bool isHidden) {
  static unsigned char val, red, redR, i, suit,rightDigit;
  static unsigned mid;
  uint8_t* pos;
  mid = 0x1213;

  if (partial == PARTIAL_LEFT) {
    CARD_BACK(1);
    hires_Draw(x,y+5,4,0,&charset[(uint16_t)(0x01 CHAR_SHIFT) + CHAR_ROW(4)]);
    hires_putc(x,y+=8,0 ,0x5f);
    hires_putc(x,y+=8,0 ,0x5f);
    hires_Draw(x,y+=8,6,0,&charset[(uint16_t)((0x5f) CHAR_SHIFT)]);
    hires_Draw(x,y+=5,5,0,&charset[(uint16_t)0x03 CHAR_SHIFT]);
    CARD_BACK(0);
  } else if (partial == PARTIAL_RIGHT) {
    ++x;
    CARD_BACK(1);
    hires_Draw(x,y+5,4,0,&charset[(uint16_t)(0x02 CHAR_SHIFT) + CHAR_ROW(4)]);
    hires_putc(x,y+=8,0 ,0x1f);
    hires_putc(x,y+=8,0 ,0x1f);
    hires_Draw(x,y+=8,6,0,&charset[(uint16_t)((0x1F) CHAR_SHIFT)]);
    hires_Draw(x,y+=5,5,0,&charset[(uint16_t)0x04 CHAR_SHIFT]);
    CARD_BACK(0);
  } else { // Full card

    switch (s[1]) {
      case 'h' : suit=0x0A; red=RED; redR=RED_RIGHT; break;
      case 'd' : suit=0x0C; red=RED; redR=RED_RIGHT; break;
      case 'c' : suit=0x0E; red=redR=0; break;
      case 's' : suit=0x10; red=redR=0; break;
      default: suit=0x7B; red=redR=0; break;
    }


    if (s[0]=='?') {
      // Overturned card: diagonal pattern back (chars 0x1e/0x1f).
      CARD_BACK(1);
      // Top edge
      hires_Draw(x,y+5,4,0,&charset[(uint16_t)(0x01 CHAR_SHIFT) + CHAR_ROW(5)]);
      hires_Draw(x+1,y+5,4,0,&charset[(uint16_t)(0x02 CHAR_SHIFT) + CHAR_ROW(5)]);

      // Middle
      hires_putcc(x,y+=8,0 ,0x1e1f);
      hires_putcc(x,y+=8,0 ,0x1e1f);
      hires_Draw(x,y+=8,7,0,&charset[(uint16_t)((0x1E) CHAR_SHIFT)]);
      hires_Draw(x+1,y,7,0,&charset[(uint16_t)((0x1F) CHAR_SHIFT)]);

      // Bottom edge
      hires_Draw(x,y+=6,4,0,&charset[(uint16_t)(0x03 CHAR_SHIFT)]);
      hires_Draw(x+1,y,4,0,&charset[(uint16_t)(0x04 CHAR_SHIFT)]);
      CARD_BACK(0);

      // Since a full overturned card is being drawn, we may have just folded.
      // Blank out the rest of the hand by it (since no double buffer is used to clear screen)
      if (x<20)
        hires_Mask(x+2,y-26,8,29,0);
      else
        hires_Mask(x-8,y-26,8,29,0);

    } else {
      // Draw full card
      rightDigit=0x13;
      switch (s[0]) {
        case 't': val=0x1c;rightDigit=0x1d; break;
        case 'j': val=0x23;  break;
        case 'q': val=0x24; break;
        case 'k': val=0x25;  break;
        case 'a': val=0x26; break;
        default:
          val=0x14+(s[0]-0x32);
      }

      // Card left edge
      if (x>0) {
#ifdef COCO3
        /* 4bpp: each char cell is BPC bytes wide; the left edge of a
           card at column x is the RIGHT pixel of the last byte of cell
           (x-1). Set its low nibble to colour 2 (black). */
        if (x==WIDTH-3) {
            /* At this position the hidden card to the right needs
               clearing - same as the original WIDTH==40 path. */
            hires_Mask(x+1,y+1,1,28,0);
        }

        pos = (uint8_t *)SCREEN + (uint16_t)(y+6) * STRIDE + (uint16_t)x * BPC - 1;
        BEGIN_GFX;
        if ((*pos & 0x0F) == 0) {
            for(i=24;i<255;--i) {
                *pos = (*pos & 0xF0) | 0x02;
                pos += STRIDE;
            }
        }
        END_GFX;
#else
        pos = (uint8_t *)SCREEN+(uint16_t)(y+6)*WIDTH+x-1;

        // Handle special cases for right side of screen
        if (x==WIDTH-3) {
             // for 32 WIDTH, make first downturned card slightly smaller
            if (WIDTH == 32 && *(pos+2)) {
                drawCardAt(WIDTH-2,y, PARTIAL_RIGHT, "??", 0);
            } else if (WIDTH==40) {
                // At this position in WIDTH 40, means drawing first card at the end of game. Clear the hidden card to the right of it
                hires_Mask(x+1,y+1,1,28,0);
            }
        }

        // Draw card left edge
        if (!*pos || *pos==0x80) {
          for(i=24;i<255;--i) {
            *(pos) += 02;
            pos+=WIDTH;
          }
        }
        else if (*(pos+WIDTH*2)==173) {
          // Make first downturned card slightly smaller for left side
          drawCardAt(x-1,y, PARTIAL_LEFT, "??", 0);
        }
#endif
      }

      xor_mask = isHidden;
      // Top edge
      hires_Draw(x,y+5,4,0,&charset[(uint16_t)(0x05 CHAR_SHIFT) + CHAR_ROW(5)]);
      hires_Draw(x+1,y+5,4,0,&charset[(uint16_t)(0x06 CHAR_SHIFT) + CHAR_ROW(5)]);

      // Card value
      hires_putc(x,y+=8, red ,val);
      hires_putc(x+1,y,  redR,rightDigit);

      // Middle
      hires_Draw(x,y+=8,7,0,&charset[(uint16_t)((mid >> 8 & 0xFF) CHAR_SHIFT)]);
      hires_Draw(x+1,y,7,0,&charset[(uint16_t)((mid & 0xFF) CHAR_SHIFT)]);

      // Suit
      hires_putc(x,y+=6,0 ,suit);
      hires_putc(x+1,y, 0,++suit);

      // Bottom edge
      hires_Draw(x,y+=8,4,0,&charset[(uint16_t)0x07 CHAR_SHIFT]);
      hires_Draw(x+1,y,4,0,&charset[(uint16_t)0x08 CHAR_SHIFT]);
      xor_mask = 0;

      // If drawing the full hand on the left side, shift the rendered cards to the right one pixel
      // to make room to draw the left edge border on the first card.
      // This only occurs on final hand flips
#ifndef COCO3
      if (x==8) {
        y+=2;
        for(i=0;i<28;i++) {
          pos = (uint8_t *)SCREEN+(uint16_t)y*WIDTH+10;
          for (x=0;x<11;x++) {
            if (x>0)
              *pos=*pos>>2;
            else
              *pos=*pos&0x03 + (*pos>>2);
            if (x<10)
              *pos+=(*(pos-1)&0x03)<<6;
            else if (i>0 && i<27)
              *pos+=0x80;
            pos--;
          }
          y--;
        }
      }
#endif
    }
  }
}

void drawCard(unsigned char x, unsigned char y, unsigned char partial, const char* s, bool isHidden) {
  y=y*8-2;
  COCO3_ADJUST_Y
  drawCardAt(x, y, partial, s, isHidden);
}

void drawLine(unsigned char x, unsigned char y, unsigned char w) {
  y=y*8;
  COCO3_ADJUST_Y
  
  if (y > 181)
    y=189;
  hires_Mask(x,y,w,2, 0b11111111);
}

void hideLine(unsigned char x, unsigned char y, unsigned char w) {
 uint8_t color=0;

  y=y*8;
  COCO3_ADJUST_Y

  if (y > 181) {
    y=189;
    #ifdef COCO3
      color=STATUS_BLANK;
    #endif
  }
   hires_Mask(x,y,w,2, color);
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
  y=y*8-1;

  // Top Corners
  hires_putc(x,y,0, 0x3b);hires_putc(x+w+1,y,0, 0x3c);

  // Accents if height > 1
  // if (h>1) {
  //   hires_putc(x+1,y+8,0, 1);
  // }

  // Top/bottom lines
  for(i=x+w;i>x;--i) {
    hires_putc(i,y,0, 0x40);
    hires_putc(i,y+(h+1)*8,0, 0x40);
  }

  // Sides
  for(i=0;i<h;++i) {
    y+=8;
    hires_putc(x,y,0, 0x3f);
    hires_putc(x+w+1,y,0, 0x3f);
  }

    // Accents if height > 1
  // if (h>1) {
  //   hires_putc(x+w,y,0, 4);
  // }

  y+=8;
  // Bottom Corners
  hires_putc(x,y,0, 0x3d);hires_putc(x+w+1,y,0, 0x3e);


}

void drawBorder() {
  static uint8_t i,y;
  static uint8_t* pos;

  //drawCardAt(12,20,FULL_CARD, "??", 0);
  //drawCardAt(13,20,FULL_CARD, "as", 0);

  // for(i=0;i<32;i+=11) {
  //   drawCardAt(i,20,FULL_CARD, "as", 0);
  //   drawCardAt(i+2,20,FULL_CARD, "as", 0);
  //   drawCardAt(i+4,20,FULL_CARD, "as", 0);
  //   drawCardAt(i+6,20,FULL_CARD, "as", 0);
  //   drawCardAt(i+8,20,FULL_CARD, "as", 0);
  // }
  // cgetc();

  drawCardAt(1,CORNER_TOP,FULL_CARD, "as", 0);
  drawCardAt(WIDTH-3,CORNER_TOP,FULL_CARD, "ah", 0);
  drawCardAt(1,CORNER_BOTTOM,FULL_CARD, "ad", 0);
  drawCardAt(WIDTH-3,CORNER_BOTTOM,FULL_CARD, "ac", 0);

  for(i=4;i<WIDTH-4;i+=4) {
    hires_putcc(i,CORNER_TOP+6,0, 0x7475);
    hires_putcc(i+2,CORNER_TOP+6,0, 0x7677);
    hires_putcc(i,CORNER_BOTTOM+24,0, 0x7475);
    hires_putcc(i+2,CORNER_BOTTOM+24,0, 0x7677);
  }
  
  // drawCardAt(5,60,FULL_CARD, "??", 0);

  // drawCardAt(10,60,PARTIAL_LEFT, "??", 0);
  // drawCardAt(11,60,FULL_CARD, "as", 0);

  // drawCardAt(15,60,PARTIAL_RIGHT, "??", 0);
  // drawCardAt(14,60,FULL_CARD, "as", 0);

}

void drawLogo() {
  static unsigned char i;
  i=4;
  drawText(WIDTH/2-5,++i, "           ");
  drawText(WIDTH/2-5,++i, " FUJI  NET ");
  drawText(WIDTH/2-5,++i, "           ");
  drawText(WIDTH/2-5,++i, "5 CARD STUD");
  drawText(WIDTH/2-5,++i, "           ");
}


void resetGraphics() {
  waitvsync();

#ifdef COCO3
  /* Restore all 16 palette entries that rgbOrComposite() saved. */
  memcpy((byte *) 0xFFB0, &paletteBackup, 16);
  width(32);
#endif

  pmode(0, 0x400);
  screen(0,0);
}


#ifdef COCO3
/* Task 1 MMU map - low half mirrors Task 0 so the running program is
   still reachable after the task swap; the high half exposes blocks
   0x34-0x37 as a 32 KB framebuffer at $8000-$FFFF. */
static const unsigned char task1MMUBlocks[8] = {
    0x38, 0x39, 0x3A, 0x3B,
    0x34, 0x35, 0x36, 0x37
};
#endif

/* setRGB / setComposite load the 16-entry palette for each output type;
   updateColors() picks one based on colorMode. */
void setRGB()
{
#ifdef COCO3
    /* RGB encoding (RrGgBb); entries 0-3 are the pmode3 charset colours. */
    *(byte *)0xFFB0 = 18;   /* 0  green felt   */
    *(byte *)0xFFB1 = 63;   /* 1  white        */
    *(byte *)0xFFB2 = 0;    /* 2  black        */
    *(byte *)0xFFB3 = 36;   /* 3  red          */
    *(byte *)0xFFB4 = 52;   /* 4  orange       */
    *(byte *)0xFFB5 = 54;   /* 5  yellow       */
    *(byte *)0xFFB6 = 4;    /* 6  dark red     */
    *(byte *)0xFFB7 = 27;   /* 7  light foam   */
    *(byte *)0xFFB8 = 7;    /* 8  dark gray    */
    *(byte *)0xFFB9 = 56;   /* 9  light gray   */
    *(byte *)0xFFBA = 28;   /* 10 teal         */
    *(byte *)0xFFBB = 9;    /* 11 sea blue     */
    *(byte *)0xFFBC = 11;   /* 12 light blue   */
    *(byte *)0xFFBD = 1;    /* 13 dark blue    */
    *(byte *)0xFFBE = 25;   /* 14 foam         */
    *(byte *)0xFFBF = 38;   /* 15 red orange   */
#endif
}

void setComposite()
{
#ifdef COCO3
    /* Composite encoding (luminance + chroma) of the same 16 colours. */
    *(byte *)0xFFB0 = 30;   /* 0  green felt   */
    *(byte *)0xFFB1 = 63;   /* 1  white        */
    *(byte *)0xFFB2 = 0;    /* 2  black        */
    *(byte *)0xFFB3 = 6;    /* 3  red          */
    *(byte *)0xFFB4 = 9;    /* 4  orange-ish   */
    *(byte *)0xFFB5 = 11;   /* 5  yellow-ish   */
    *(byte *)0xFFB6 = 5;    /* 6  dark red     */
    *(byte *)0xFFB7 = 31;   /* 7  light foam   */
    *(byte *)0xFFB8 = 16;   /* 8  dark gray    */
    *(byte *)0xFFB9 = 48;   /* 9  light gray   */
    *(byte *)0xFFBA = 26;   /* 10 teal         */
    *(byte *)0xFFBB = 23;   /* 11 sea blue     */
    *(byte *)0xFFBC = 21;   /* 12 light blue   */
    *(byte *)0xFFBD = 22;   /* 13 dark blue    */
    *(byte *)0xFFBE = 28;   /* 14 foam         */
    *(byte *)0xFFBF = 7;    /* 15 red orange   */
#endif
}

void rgbOrComposite() {
#ifdef COCO3
    if (!isCoCo3)
        return;

    memcpy(&paletteBackup, (byte *) 0xFFB0, 16);

    /* Set up Task 1 MMU: program/stack visible at $0000-$7FFF, framebuffer
       at $8000-$FFFF. */
    memcpy((void *)0xFFA8, (const void *)task1MMUBlocks, 8);

    /* Clear the framebuffer (32000 bytes) under Task 1 with IRQs off. */
    BEGIN_GFX;
    memset((void *)0x8000, 0x00, 32000);
    END_GFX;

    /* Load RGB palette first so the prompt is legible on either monitor. */
    setRGB();

    asm { sync }

    *(byte *)0xFF90 = 0x4C;             /* CoCo3-native, MMU on, ROM int. */
    *(byte *)0xFF98 = 0x80;             /* graphics mode                  */
    *(byte *)0xFF9A = 0;                /* border = palette index 0 (felt) */
    *(byte *)0xFF99 = 0x3E;             /* 200 lines, 160 B/row, 16 col   */
    *(uint16_t *)0xFF9D = 0xD000;       /* video start = block 0x34       */
    *(byte *)0xFF9F = 0;                /* no horizontal scroll / HVEN    */

    if (colorMode == 0) {
        drawTextAt(10, 96, "R-GB OR C-OMPOSITE");
        while (!colorMode) {
            switch (cgetc()) {
                case 'R': case 'r': colorMode = 1; break;
                case 'C': case 'c': colorMode = 2; break;
            }
        }
    }

    updateColors();
#endif
}

#ifdef COCO3
/* Recolour two pmode3 glyphs for the 16-colour build: the dash's red
   pixels become black, and the clock's 1/2 pixels swap so it reads as a
   white face with a black hand. */
static void patchFontForCoco3(void)
{
    static const struct { uint8_t ch; uint8_t map[4]; } patches[] = {
        { 0x2D, { 0, 1, 2, 2 } },   /* dash: 3 -> 2          */
        { 0x28, { 0, 2, 1, 3 } }    /* clock: swap 1 <-> 2   */
    };
    const uint8_t glyph_bytes = 16;
    uint8_t i, r, pos, pix, b, result;
    for (i = 0; i < sizeof(patches)/sizeof(patches[0]); i++) {
        const uint8_t *map = patches[i].map;
        uint16_t base = (uint16_t)patches[i].ch  CHAR_SHIFT;
        for (r = 0; r < glyph_bytes; r++) {
            b = charset[base + r];
            result = 0;
            for (pos = 0; pos < 4; pos++) {
                pix = (b >> (6 - 2*pos)) & 3;
                result |= map[pix] << (6 - 2*pos);
            }
            charset[base + r] = result;
        }
    }
}
#endif

void initGraphics() {

    initCoCoSupport();

#ifdef COCO3
    /* Patch the font before rgbOrComposite(), whose prompt uses the
       dash and clock glyphs. */
    patchFontForCoco3();

    /* rgbOrComposite() builds the 320x200x16 GIME mode and clears the
       framebuffer; the calls below draw into it via hires_*. */
    rgbOrComposite();
    clearStatusBar();
    resetScreen();
#else
    pmode(3,SCREEN);
    pcls(0);
    screen(1,0);

    clearStatusBar();
    resetScreen();

    rgbOrComposite();
#endif
}

#endif
