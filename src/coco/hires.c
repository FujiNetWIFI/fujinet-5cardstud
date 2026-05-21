#ifdef _CMOC_VERSION_

#include <cmoc.h>
#include <coco.h>
#include "../platform-specific/sound.h"
#include "hires.h"
#include "../misc.h"
#include "vars.h"

extern uint8_t charset[];

extern uint8_t xor_mask;
extern uint8_t font_shift;
#ifdef COCO3
extern uint8_t card_back;
/* Recolours card-back glyphs when card_back is set: source 2bpp pixel
   -> 4bpp palette index. Pixel 0 stays felt so rounded corners show. */
static const uint8_t card_back_map[4] = {
     0,   /* felt  -> felt      */
     1,   /* white -> white     */
     2,   /* black -> black     */
    11    /* red   -> sea blue  */
};
#endif

#ifdef COCO3
/* CoCo 3 4bpp drawing primitives. x is a character column (cell = BPC
   bytes), y a scanline. Glyphs are the native 8px-wide 16-byte/glyph
   charset (2 source bytes per row). All framebuffer writes run with
   Task 1 selected (BEGIN_GFX..END_GFX) so logical $8000-$FFFF maps to
   the video blocks 0x34-0x37. */

/*-----------------------------------------------------------------------*/
void hires_putc(uint8_t x, uint8_t y, uint8_t rop, uint8_t c)
{
  hires_Draw(x, y, 9, rop, &charset[(uint16_t)c CHAR_SHIFT]);
}

/*-----------------------------------------------------------------------*/
void hires_putcc(uint8_t x, uint8_t y, uint8_t rop, uint16_t cc)
{
  hires_putc(x,   y, rop, (uint8_t)(cc >> 8));
  hires_putc(++x, y, rop, (uint8_t)cc);
}

/*-----------------------------------------------------------------------*/
void hires_Mask(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t c)
{
  uint8_t *pos = (uint8_t *)SCREEN + (uint16_t)y * STRIDE + (uint16_t)x * BPC;
  BEGIN_GFX;
  ylen++;
  while (--ylen) {
    memset(pos, c, (uint16_t)xlen * BPC);
    pos += STRIDE;
  }
  END_GFX;
}

/*-----------------------------------------------------------------------*/
/* 16-byte glyph: each row is 2 source bytes (8 px 2bpp) emitted as
   4 dest bytes (8 px 4bpp). */
void hires_Draw(uint8_t x, uint8_t y, uint8_t ylen, uint8_t rop, uint8_t *src)
{
  uint8_t *pos = (uint8_t *)SCREEN + (uint16_t)y * STRIDE + (uint16_t)x * BPC;
  uint8_t s0, s1, p0, p1, p2, p3, p4, p5, p6, p7;

  BEGIN_GFX;
  if (card_back) {
    while (--ylen) {
      s0 = *src++ | rop;
      s1 = *src++ | rop;
      p0 = card_back_map[(s0 >> 6) & 3];
      p1 = card_back_map[(s0 >> 4) & 3];
      p2 = card_back_map[(s0 >> 2) & 3];
      p3 = card_back_map[s0 & 3];
      p4 = card_back_map[(s1 >> 6) & 3];
      p5 = card_back_map[(s1 >> 4) & 3];
      p6 = card_back_map[(s1 >> 2) & 3];
      p7 = card_back_map[s1 & 3];
      pos[0] = (p0 << 4) | p1;
      pos[1] = (p2 << 4) | p3;
      pos[2] = (p4 << 4) | p5;
      pos[3] = (p6 << 4) | p7;
      pos += STRIDE;
    }
  } else {
    while (--ylen) {
      s0 = *src++ | rop;
      s1 = *src++ | rop;
      p0 = (s0 >> 6) & 3;
      p1 = (s0 >> 4) & 3;
      p2 = (s0 >> 2) & 3;
      p3 = s0 & 3;
      p4 = (s1 >> 6) & 3;
      p5 = (s1 >> 4) & 3;
      p6 = (s1 >> 2) & 3;
      p7 = s1 & 3;
      pos[0] = (p0 << 4) | p1;
      pos[1] = (p2 << 4) | p3;
      pos[2] = (p4 << 4) | p5;
      pos[3] = (p6 << 4) | p7;
      pos += STRIDE;
    }
  }
  END_GFX;
}

#else  /* CoCo 1/2 - original 2bpp PMODE 3 drawing */

/*-----------------------------------------------------------------------*/
void hires_putc(uint8_t x, uint8_t y, uint8_t rop, uint8_t c)
{
  hires_Draw(x,y,9,rop,&charset[(uint16_t)c CHAR_SHIFT]);
}

/*-----------------------------------------------------------------------*/
void hires_putcc(uint8_t x, uint8_t y,uint8_t rop, uint16_t cc)
{
    // CMOC _REALLY_ hates this.
    hires_putc(x,y,rop,cc >> 8 & 0xFF);
    hires_putc(++x,y,rop,cc & 0xFF);
}

void hires_Mask(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t c)
{
  uint8_t *pos = (uint8_t *)SCREEN+(uint16_t)y*WIDTH+x;
  ylen++;
  while (--ylen) {
    memset(pos,c,xlen);
    pos+=WIDTH;
  }
}

void hires_Draw(uint8_t x, uint8_t y, uint8_t ylen, uint8_t rop, uint8_t *src)
{
  uint8_t *pos = (uint8_t *)SCREEN+(uint16_t)y*WIDTH+x;
  while (--ylen) {
    *pos=(*(src++)|rop);
    if (xor_mask)
      *pos^=*(src+86*8-1);
    pos+=WIDTH;
  }
}

#endif

#endif
