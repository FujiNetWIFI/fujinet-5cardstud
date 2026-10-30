; ram.inc -- where everything lives.
;
; Internal RAM (64 bytes):
;   $00-$07  register bank 0: the main program
;   $08-$17  the stack, 8 levels: the interrupts take up to 3
;   $18-$1F  register bank 1: the interrupts' own (r5: the next band, r4:
;            the staged row's Y, from the vblank to the last row)
;   $20-$3C  below
;   $3D-$3F  the BIOS's (its keyboard and IRQ routines)

ROWBUF  equ 020h                ; ROW_TAB: the next table row, staged
FRAMES  equ 038h                ; vblanks, for the main loop's clock
QUIET   equ 039h                ; frames whose last band has run
MODE    equ 03Ah                ; MODE_*
SNDCNT  equ 03Bh                ; the sound: frames left (fixed.inc beep)
CURPG   equ 03Ch                ; the page mapped at $800

    if ROWBUF+ROW_TAB > FRAMES
    error "ROWBUF has grown into FRAMES"
    endif

MODE_SETUP equ 002h             ; a new screen's objects (fixed.inc obj_setup)
MODE_TEXT  equ 004h             ; ... with the rows' singles side by side

; External RAM (128 bytes, $00-$7F, MOVX with P1_MAIN):
LINEBUF equ 000h                ; 16: a text line being built (disp.inc)
TXCOL   equ 010h                ; its colour, and for characters with bit 7 set
TXCOL2  equ 011h
OLDPG   equ 012h                ; farcall's caller's page
TMP0    equ 013h                ; a loop's index across calls that keep nothing
TMP1    equ 014h
; the input (input.inc)
KLAST   equ 015h                ; BIOS readkey's memory (its r7)
KEY     equ 016h                ; the key pressed this frame (K_NONE: none)
JOY     equ 017h                ; the stick now: J_* bits
JOYP    equ 018h                ; last frame's
JOYEV   equ 019h                ; pressed since last frame
REP     equ 01Ah                ; auto-repeat countdown
; a row being built (table.inc)
CARDS   equ 01Bh                ; 10: a hand, 5 x (rank, suit), wire ASCII
ROWNAME equ 025h                ; 2: the row's name, its characters ...
ROWNC   equ 027h                ; ... and their colour
ROWN    equ 028h                ; the row
SCR0    equ 029h                ; scratch, 8 bytes: per-routine
GRIDS   equ 031h                ; 5: each card place's rows (grid bits)
