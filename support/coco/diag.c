#include <cmoc.h>
#include <coco.h>

/*
  Phase 1 text-rendering test. Uses 5cardstud's pmode3 charset converted
  to 4bpp (32 bytes/glyph, 8x8 pixels) and the standard CoCo 3 320x200x16
  graphics mode that fujitzee and battleship use. Palette is written via
  explicit byte stores - cmoc's memcpy was leaving the registers at the
  startup defaults for some reason.

  Source pixel values 0-3 land directly in palette indices 0-3:
    0 = green felt (table background)
    1 = white      (card background)
    2 = black      (text / outlines)
    3 = red        (hearts / diamonds)
*/

extern unsigned char charset_4bpp[];

#define SCREEN ((unsigned char *)0x8000)

#define task0() do { asm("clr",  "$FF91"); } while (0)
#define task1() do { asm("ldb",  "#1"); asm("stb", "$FF91"); } while (0)
#define BEGIN_GFX  disableInterrupts(); task1()
#define END_GFX    task0(); enableInterrupts()

static const unsigned char task1MMU[8] = {
    0x38, 0x39, 0x3A, 0x3B,
    0x34, 0x35, 0x36, 0x37
};

static void setPalette(void)
{
    *(unsigned char *)0xFFB0 = 18;   /* 0 green felt    */
    *(unsigned char *)0xFFB1 = 63;   /* 1 white         */
    *(unsigned char *)0xFFB2 = 0;    /* 2 black         */
    *(unsigned char *)0xFFB3 = 36;   /* 3 red           */
    *(unsigned char *)0xFFB4 = 52;   /* 4 orange        */
    *(unsigned char *)0xFFB5 = 54;   /* 5 yellow        */
    *(unsigned char *)0xFFB6 = 4;    /* 6 dark red      */
    *(unsigned char *)0xFFB7 = 27;   /* 7 light foam    */
    *(unsigned char *)0xFFB8 = 7;    /* 8 dark gray     */
    *(unsigned char *)0xFFB9 = 56;   /* 9 light gray    */
    *(unsigned char *)0xFFBA = 28;   /* 10 teal         */
    *(unsigned char *)0xFFBB = 9;    /* 11 sea blue     */
    *(unsigned char *)0xFFBC = 11;   /* 12 light blue   */
    *(unsigned char *)0xFFBD = 1;    /* 13 dark blue    */
    *(unsigned char *)0xFFBE = 25;   /* 14 foam         */
    *(unsigned char *)0xFFBF = 38;   /* 15 red orange   */
}

static void hires_putc(unsigned char x, unsigned char y, unsigned char c)
{
    const unsigned char *src = charset_4bpp + ((unsigned)c << 5);
    unsigned char *pos = (unsigned char *)SCREEN
                       + (unsigned)y * 1280
                       + (unsigned)x * 4;
    unsigned char row;

    BEGIN_GFX;
    for (row = 0; row < 8; row++)
    {
        pos[0] = src[0];
        pos[1] = src[1];
        pos[2] = src[2];
        pos[3] = src[3];
        src += 4;
        pos += 160;
    }
    END_GFX;
}

static void hires_puts(unsigned char x, unsigned char y, const char *s)
{
    while (*s)
    {
        hires_putc(x++, y, (unsigned char)*s);
        s++;
    }
}

void main(void)
{
    initCoCoSupport();

    memcpy((void *)0xFFA8, (const void *)task1MMU, 8);

    /* Clear framebuffer to felt colour (palette idx 0 packed = 0x00) */
    BEGIN_GFX;
    memset(SCREEN, 0x00, 32000);
    END_GFX;

    setPalette();

    asm { sync }

    *(unsigned char *)0xFF90 = 0x4C;
    *(unsigned char *)0xFF98 = 0x80;
    *(unsigned char *)0xFF9A = 0;
    *(unsigned char *)0xFF99 = 0x3E;
    *(unsigned int  *)0xFF9D = 0xD000;
    *(unsigned char *)0xFF9F = 0;

    hires_puts(2,  2,  "5 CARD STUD - COCO 3 PORT");
    hires_puts(2,  3,  "PHASE 1 TEXT TEST");

    hires_puts(2,  6,  "STANDARD 320X200X16 MODE");
    hires_puts(2,  7,  "ATARI 5CARDSTUD COLORS");
    hires_puts(2,  8,  "PMODE3 CHARSET CONVERTED");

    hires_puts(2,  11, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    hires_puts(2,  13, "0123456789  ?,.+-=*/()<>");

    hires_puts(2,  16, "IF READABLE, PHASE 1 WORKS");

    for (;;)
        ;
}
