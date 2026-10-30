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

; External RAM (128 bytes, $00-$7F, MOVX with P1_MAIN), every byte of it:
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
; the request in flight (net.inc)
NSTATE  equ 036h                ; NS_*
NSEQ    equ 037h                ; the sequence number posted
NWAIT   equ 038h                ; frames without progress
ACT     equ 039h                ; the action queued (0: none, a poll)
ACTARG  equ 03Ah                ; its argument
ACTSENT equ 03Bh                ; the action under way
NARG    equ 03Ch                ; its argument
NBWL    equ 03Dh                ; 2: STATUS's last bytes-waiting
NETHAVE equ 03Fh                ; how it ended: NH_*
RXLEN   equ 040h                ; 2: the reply's length
; who and where
SCNAME  equ 042h                ; 9: the player's name, NUL-terminated
SCTABL  equ 04Bh                ; 9: the table's id
; the boot's, and at the table the same bytes again
SEL     equ 054h                ; a list's selection
NTBL    equ 055h                ; tables listed
NAMEN   equ 056h                ; the name typed so far
T1KEY   equ SEL                 ; the move T1's menu shows (0FFh: draw it)
RNEXT   equ NTBL                ; lastResult's next page in T1
LPOT    equ NAMEN               ; the pot's low byte at the last state
ROWSNEW equ TMP1                ; a row redrawn this state (page 3, for 6)
; the table: what a tick needs, taken at state time (the reply window is
; repainted once the next request is under way)
NSEAT   equ 057h                ; playerCount
ROUND   equ 058h                ; 0 waiting, 1-4 the streets, 5 showdown
ACTV    equ 059h                ; the seat to move (0FFh: nobody)
MOVET   equ 05Ah                ; its seconds left
NMOV    equ 05Bh                ; our valid moves (on our turn)
MOVES   equ 05Ch                ; 5 x 2: their codes ("ch", "bl" ...)
MAMT    equ 066h                ; 5: their amounts (0: none)
MSEL    equ 06Bh                ; the move picked
MOVEC   equ 06Ch                ; 2: the move being sent (ACT_MOVE)
SPOT    equ 06Eh                ; the seat T1 describes
POLLW   equ 06Fh                ; frames until the next poll
SECF    equ 070h                ; frames to the clock's next second
LROUND  equ 071h                ; the round the screen shows (0FFh: none)
RSIG    equ 072h                ; 8: each row's signature (table.inc)
RPAGE   equ 07Ah                ; lastResult's page in T1, and its clock
RPCLK   equ 07Bh
HELP    equ 07Ch                ; 1: the help screen is up; 2: closing
OPPOFF  equ 07Dh                ; 7 opponents: the first one on row 0 (1, 2)
TMP2    equ 07Eh
LACTV   equ 07Fh                ; the seat to move the screen shows
