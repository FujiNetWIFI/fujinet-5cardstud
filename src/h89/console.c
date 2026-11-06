#ifdef __H89__

/**
 * @brief   H19 terminal screen - Heathkit H89
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * The console is the H19 on the H89's 8250 at 9600 baud -- under a thousand
 * characters a second -- and the shared game code repaints the whole table on
 * every server poll. So drawing goes into a back buffer (buf), a second buffer
 * (scr) mirrors what the terminal shows, and flush() sends only the cells
 * that differ. Console bytes go through the OS raw: HDOS's .SCOUT/.SCIN with
 * the console driver in raw character mode, or the CP/M BIOS's
 * CONOUT/CONST/CONIN straight from the jump table (the BDOS console calls
 * expand, filter and hold back characters). The OS layer is
 * fujinet-battleship's src/h89/console.c.
 */

#include <string.h>
#include "console.h"

#define ESC 0x1B
#define CUR_UNKNOWN 0xFF

// Re-send up to this many unchanged cells rather than address past them
// (ESC Y row col is four bytes)
#define GAP_MAX 3

// Erase the rest of a row (ESC K, two bytes plus maybe an address) once at
// least this many cells there would otherwise have to be sent as spaces
#define EOL_MIN 4

// Clear the screen (ESC E) once at least this many cells are to be blanked
#define CLEAR_MIN 600

static uint8_t buf[SCR_W * SCR_H];   // what is being drawn
static uint8_t scr[SCR_W * SCR_H];   // what the terminal shows
static uint8_t dirty[SCR_H];         // rows of buf touched since the last flush
static uint8_t changed[SCR_H];       // rows flush() found differing

// Row starts: sccz80 multiplies through a library call, and put() runs a
// couple of thousand times a screen on a 2 MHz Z80
static const uint16_t rowOff[SCR_H] = {
    0 * SCR_W, 1 * SCR_W, 2 * SCR_W, 3 * SCR_W, 4 * SCR_W,
    5 * SCR_W, 6 * SCR_W, 7 * SCR_W, 8 * SCR_W, 9 * SCR_W,
    10 * SCR_W, 11 * SCR_W, 12 * SCR_W, 13 * SCR_W, 14 * SCR_W,
    15 * SCR_W, 16 * SCR_W, 17 * SCR_W, 18 * SCR_W, 19 * SCR_W,
    20 * SCR_W, 21 * SCR_W, 22 * SCR_W, 23 * SCR_W, 24 * SCR_W,
};

// What the terminal is doing now: cursor, reverse video, graphics mode.
// CUR_UNKNOWN forces the next cell to set it.
static uint8_t curX, curY, curRev, curGfx;

/*
 * Raw console I/O
 */

#ifdef __HDOS__

// HDOS system calls: RST 38H, then the function byte. HDOS is 8080 code and
// leaves IX and IY alone.
void conOut(uint8_t c) __z88dk_fastcall __naked
{
#asm
    ld      a,l
    rst     38h
    defb    2           ; .SCOUT
    ret
#endasm
}

int conIn(void) __naked
{
#asm
    rst     38h
    defb    1           ; .SCIN - carry set when nothing is waiting
    ld      hl,-1
    ret     c
    ld      l,a
    ld      h,0
    ret
#endasm
}

// .CONSL: A = index of the console byte, B = the bits to set, C = the mask of
// bits to change
uint8_t h89ConslIndex, h89ConslBits, h89ConslMask;

static void consl(void) __naked
{
#asm
    ld      a,(_h89ConslBits)
    ld      b,a
    ld      a,(_h89ConslMask)
    ld      c,a
    ld      a,(_h89ConslIndex)
    rst     38h
    defb    6           ; .CONSL
    ret
#endasm
}

static void conslSet(uint8_t index, uint8_t bits, uint8_t mask)
{
    h89ConslIndex = index;
    h89ConslBits = bits;
    h89ConslMask = mask;
    consl();
}

// The five console cells (S.CSLMD..S.CONFL) live at 040.326A in HDOS 2.0's
// system values (HOSDEF: S.VAL at 040.277A)
#define S_CSLMD ((volatile uint8_t *)0x20D6)
#define I_CSLMD 0
#define I_CONTY 1
#define I_CUSOR 2
#define I_CONWI 3
#define I_CONFL 4

#define CSL_ECH 0x80 // suppress echo
#define CSL_RAW 0x04 // raw mode I/O (no CTL-C/S/O/P processing)
#define CSL_WRP 0x02 // wrap lines at width
#define CSL_CHR 0x01 // character mode
#define CTP_MLI 0x20 // map lower case to upper on input
#define CTP_MLO 0x10 // map lower case to upper on output
#define CTP_BKM 0x02 // map backspace to rubout on input
#define CO_FLG 0x01  // CTL-O (discard output) flag
#define CS_FLG 0x80  // CTL-S (hold output) flag

static uint8_t savedConsole[5];

static void osInit(void)
{
    memcpy(savedConsole, (void *)S_CSLMD, sizeof(savedConsole));

    // Raw character mode with no echo, no wrap and no width (HDOS counts
    // escape sequences as printing columns), and no case mapping: the
    // graphics characters are sent as lower case letters.
    conslSet(I_CSLMD, CSL_ECH | CSL_RAW | CSL_CHR, CSL_ECH | CSL_RAW | CSL_WRP | CSL_CHR);
    conslSet(I_CONTY, 0, CTP_MLI | CTP_MLO | CTP_BKM);
    conslSet(I_CONWI, 255, 0xFF);
    conslSet(I_CONFL, 0, CO_FLG | CS_FLG);
}

static void osExit(void)
{
    conslSet(I_CSLMD, savedConsole[I_CSLMD], 0xFF);
    conslSet(I_CONTY, savedConsole[I_CONTY], 0xFF);
    conslSet(I_CONWI, savedConsole[I_CONWI], 0xFF);
    conslSet(I_CUSOR, 0, 0xFF); // ESC z homed the cursor
}

#else /* CP/M */

// The BIOS jump table: (0001) points at its WBOOT entry, base+3. CONST is at
// +3 from there, CONIN +6, CONOUT +9. The BIOS may use any register, so IX
// and IY are kept for the C side.
void conOut(uint8_t c) __z88dk_fastcall __naked
{
#asm
    EXTERN  l_dcal
    push    ix
    push    iy
    ld      c,l
    ld      hl,(1)
    ld      de,9
    add     hl,de
    call    l_dcal      ; CONOUT
    pop     iy
    pop     ix
    ret
#endasm
}

int conIn(void) __naked
{
#asm
    EXTERN  l_dcal
    push    ix
    push    iy
    ld      hl,(1)
    ld      de,3
    add     hl,de
    call    l_dcal      ; CONST: A = FF when a character is waiting
    or      a
    ld      hl,-1
    jr      z,conIn_done
    ld      hl,(1)
    ld      de,6
    add     hl,de
    call    l_dcal      ; CONIN
    ld      l,a
    ld      h,0
conIn_done:
    pop     iy
    pop     ix
    ret
#endasm
}

static void osInit(void)
{
}

static void osExit(void)
{
}

#endif /* __HDOS__ */

static void esc(uint8_t c)
{
    conOut(ESC);
    conOut(c);
}

static void forget(void)
{
    curX = curY = curRev = curGfx = CUR_UNKNOWN;
}

static void moveTo(uint8_t x, uint8_t y)
{
    if (x != curX || y != curY)
    {
        esc('Y');
        conOut(32 + y);
        conOut(32 + x);
        curX = x;
        curY = y;
    }
}

/// @brief Graphics mode a cell needs: 1 on, 0 off, 2 either
static uint8_t gfxNeed(uint8_t code)
{
    code &= 0x7F;
    if (code < 0x20 || code == 0x7F)
        return 1;
    // Graphics mode only remaps 0x5E-0x7E: everything below prints the same
    // either way
    return code >= 0x5E ? 0 : 2;
}

/// @brief Send a cell at the cursor, setting the modes it needs
static void sendCell(uint8_t code)
{
    static uint8_t g, need;

    if ((code & REV) != curRev)
    {
        curRev = code & REV;
        esc(curRev ? 'p' : 'q');
    }

    need = gfxNeed(code);
    if (need != 2 && need != curGfx)
    {
        curGfx = need;
        esc(need ? 'F' : 'G');
    }

    g = code & 0x7F;
    if (need == 1)
    {
        // Graphics characters: ROM 0x00-0x1E are ` through ~, 0x1F is _,
        // the bullet is ^
        if (g == 0x7F)
            g = '^';
        else if (g == 0x1F)
            g = '_';
        else
            g += 0x60;
    }

    conOut(g);

    // In discard-at-end-of-line mode the cursor stays on the last column
    curX = (curX == SCR_W - 1) ? CUR_UNKNOWN : curX + 1;
}

/// @brief Can the cells from the cursor up to x be re-sent with no mode
/// change? They already show on screen, so this only moves the cursor.
static bool cheapGap(const uint8_t *row, uint8_t x)
{
    static uint8_t cx, need;

    if (curX == CUR_UNKNOWN || curX >= x || x - curX > GAP_MAX)
        return false;

    for (cx = curX; cx < x; cx++)
    {
        if ((row[cx] & REV) != curRev)
            return false;
        need = gfxNeed(row[cx]);
        if (need != 2 && need != curGfx)
            return false;
    }
    return true;
}

static void clearTerminal(void)
{
    // Home first: ESC E on the 25th line clears only that line
    curX = curY = CUR_UNKNOWN;
    moveTo(0, 0);
    esc('E');
    memset(scr, ' ', SCR_W * STATUS_ROW);
}

static void flushRow(uint8_t y)
{
    static uint8_t *b, *s;
    static uint8_t x, last, stale;

    b = buf + rowOff[y];
    s = scr + rowOff[y];

    // The row's last non-blank cell, and how many cells past it still show
    // something on screen
    last = SCR_W;
    while (last && b[last - 1] == ' ')
        last--;
    stale = 0;
    for (x = last; x < SCR_W; x++)
        if (s[x] != ' ')
            stale++;
    if (stale < EOL_MIN)
        last = SCR_W;

    for (x = 0; x < last; x++)
    {
        if (b[x] == s[x])
            continue;

        if (y == curY && cheapGap(b, x))
        {
            while (curX < x)
                sendCell(b[curX]);
        }
        else
            moveTo(x, y);

        sendCell(b[x]);
        s[x] = b[x];
    }

    if (last < SCR_W)
    {
        moveTo(last, y);
        esc('K');
        memset(s + last, ' ', SCR_W - last);
    }
}

/// @brief Cells of the changed rows that are to go blank
static uint16_t blanksToSend(void)
{
    static uint8_t y, n;
    static uint8_t *b, *s;
    static uint16_t blanks;

    blanks = 0;
    for (y = 0; y < STATUS_ROW; y++)
    {
        if (!changed[y])
            continue;
        b = buf + rowOff[y];
        s = scr + rowOff[y];
        for (n = SCR_W; n; n--)
        {
            if (*b == ' ' && *s != ' ')
                blanks++;
            b++;
            s++;
        }
    }
    return blanks;
}

void flush(void)
{
    static uint8_t y, rows;

    // Which rows really differ (the shared code repaints the whole table
    // every poll, so most dirty rows come out the same)
    rows = 0;
    for (y = 0; y < SCR_H; y++)
    {
        changed[y] = dirty[y] && memcmp(buf + rowOff[y], scr + rowOff[y], SCR_W);
        dirty[y] = 0;
        rows += changed[y];
    }
    if (!rows)
        return;

    // A screen change: clear and draw rather than overwrite with spaces
    if (rows >= STATUS_ROW / 2 && blanksToSend() >= CLEAR_MIN)
    {
        clearTerminal();
        for (y = 0; y < STATUS_ROW; y++)
            changed[y] = memcmp(buf + rowOff[y], scr + rowOff[y], SCR_W) != 0;
    }

    for (y = 0; y < SCR_H; y++)
        if (changed[y])
            flushRow(y);
}

/*
 * Screen
 */

void termInit(void)
{
    osInit();

    esc('G');      // graphics mode off
    esc('q');      // reverse video off
    esc('x');      // cursor off
    conOut('5');
    esc('w');      // discard (no wrap) at the end of a line
    esc('t');      // keypad shifted: the unshifted keypad sends ESC A-D
    esc('x');      // the 25th line, for the status bar
    conOut('1');
    forget();
    curGfx = 0;
    curRev = 0;

    clearTerminal();
    moveTo(0, STATUS_ROW);
    esc('K');
    memset(scr + SCR_W * STATUS_ROW, ' ', SCR_W);
    memset(buf, ' ', sizeof(buf));
    memset(dirty, 0, sizeof(dirty));
}

void termExit(void)
{
    static uint16_t t;

    // Back to the power-up configuration the DIP switches set: modes,
    // cursor, keypad, 25th line off, and a clear screen. The terminal needs
    // a moment.
    esc('z');
    t = TICKS;
    while ((uint16_t)(TICKS - t) < 100)
        ;
    osExit();
}

void termBell(void)
{
    conOut(0x07);
}

void put(uint8_t x, uint8_t y, uint8_t code)
{
    if (x >= SCR_W || y >= SCR_H)
        return;

    buf[rowOff[y] + x] = code;
    dirty[y] = 1;
}

void fill(uint8_t x, uint8_t y, uint8_t w, uint8_t code)
{
    if (x >= SCR_W || y >= SCR_H)
        return;
    if (w > SCR_W - x)
        w = SCR_W - x;

    memset(buf + rowOff[y] + x, code, w);
    dirty[y] = 1;
}

void blankRows(uint8_t y, uint8_t n)
{
    memset(buf + rowOff[y], ' ', (uint16_t)n * SCR_W);
    memset(dirty + y, 1, n);
}

uint8_t peek(uint8_t x, uint8_t y)
{
    if (x >= SCR_W || y >= SCR_H)
        return ' ';
    return buf[rowOff[y] + x];
}

#endif /* __H89__ */
