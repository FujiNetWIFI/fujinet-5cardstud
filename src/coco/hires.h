#ifdef _CMOC_VERSION_

#ifndef HIRES_H
#define HIRES_H

#include <cmoc.h>
#include <coco.h>

void hires_putc(uint8_t x, uint8_t y, uint8_t rop, uint8_t c);
void hires_putcc(uint8_t x, uint8_t y,uint8_t rop, unsigned cc);
void hires_Mask(uint8_t x, uint8_t y, uint8_t xlen, uint8_t ylen, uint8_t c);
void hires_Draw(uint8_t x, uint8_t y, uint8_t ylen, uint8_t rop, uint8_t *src);

#ifdef COCO3
/* The CoCo 3 build runs in 320x200x16 mode with a 32 KB framebuffer
   exposed at $8000 via MMU Task 1 (blocks 0x34-0x37). All framebuffer
   writes must happen with Task 1 selected and IRQs masked; BEGIN_GFX
   / END_GFX bracket each region. */
#define SCREEN ((uint8_t*)0x8000U)
#define task0() do { asm("clr",  "$FF91"); } while (0)
#define task1() do { asm("ldb",  "#1"); asm("stb", "$FF91"); } while (0)
#define BEGIN_GFX  disableInterrupts(); task1()
#define END_GFX    task0(); enableInterrupts()
#else
#define SCREEN ((uint8_t*)0x6000U)
#define BEGIN_GFX
#define END_GFX
#endif

#endif /* HIRES_H */
#endif
