; equ.inc -- constants: the console, the cartridge, the screen, the server.

; ---- P1 (all active low but COPY and LUM) ----
; Main code runs with external RAM and the cartridge on the bus; the interrupts
; switch to the VDC and put P1 back. COPY low always, or the cart's write
; strobe is gated off. The bank bits are left high: APPBANK pages are picked
; through the mailbox, not by P10/P11.
P1_MAIN equ 0AFh                ; XRAM + cart selected, VDC parked
P1_VDC  equ 0B7h                ; VDC selected
P1_JOY  equ 0BBh                ; keyboard scanner (joystick) selected

; ---- the mailbox (fuji_mailbox.h, protocol 2) ----
FN_W_REGSEL     equ 0E0h
FN_W_REGDATA    equ 0E1h
FN_W_DATA       equ 0E2h
FN_W_SEQ        equ 0E3h
FN_REG_DEVICE   equ 000h
FN_REG_CMD      equ 001h
FN_REG_NPARAM   equ 002h
FN_REG_DATA_RST equ 005h
FN_REG_CODEBANK equ 008h
FN_REG_POKE_LO  equ 00Ah
FN_REG_POKE_HI  equ 00Bh
FN_REG_POKE     equ 00Ch
FN_REG_RXWIN    equ 00Dh
FN_R_ACKSEQ     equ 0F20h
FN_R_ERR        equ 0F22h
FN_R_REPLY_CMD  equ 0F23h
FN_R_RXLEN_LO   equ 0F24h
FN_R_RXLEN_HI   equ 0F25h
FN_R_MAGIC0     equ 0F29h
FN_R_MAGIC1     equ 0F2Ah
FN_R_PROTO_VER  equ 0F2Bh
FN_R_DATA       equ 0F30h       ; the read window: 208 bytes from RXWIN*16
FN_PROTO_VER    equ 2
O2_APP_HDR      equ 0F30h
O2_APP_VERSION  equ 1

FUJI_DEVICEID   equ 070h
FC_ACK          equ 006h
FC_AKOPEN       equ 0DCh        ; OPEN_APPKEY
FC_AKREAD       equ 0DDh        ; READ_APPKEY
FC_AKWRITE      equ 0DEh        ; WRITE_APPKEY
NET_DEVICEID    equ 071h
NC_OPEN         equ 04Fh
NC_CLOSE        equ 043h
NC_READ         equ 052h
NC_STATUS       equ 053h
NM_GET          equ 00Ch        ; open mode: HTTP GET
NS_EOF          equ 136         ; nDevStatus: end of file

; ---- the VDC's character generator (o2doc appendix C) ----
G_SPACE equ 12
G_DOT   equ 39
G_DASH  equ 40
G_BLOCK equ 47
G_TEN   equ 48                  ; "10" in one cell
CH_BALL  equ 00h                ; the pictures' codes in text (mkcharmap.py)
CH_BLOCK equ 08h
CH_ARROW equ 0Ah
CH_TEN   equ 0Bh

; Character colours, as MAME's i8244 draws them (control byte bits 1-3).
; A character can't be black: 0 is the darkest, a dark grey.
C_DGREY  equ 0
C_RED    equ 1
C_GREEN  equ 2
C_TAN    equ 3
C_BLUE   equ 4
C_VIOLET equ 5
C_CYAN   equ 6
C_WHITE  equ 7

; The colour register ($A3): bits 5-3 the background (MAME bitswap 3,4,5),
; bits 6 and 2-0 the grid (bitswap 6,0,1,2).
BG_BLACK equ 000h
BG_FELT  equ 010h               ; green
GRID_WHITE equ 047h
COL_TEXT equ BG_BLACK
COL_TABLE equ BG_FELT|GRID_WHITE

; The control register ($A0) on the table screen. The cards are the grid in
; fill mode: a vertical segment is a whole 16 x 24 cell, white.
CTL_TOP equ vdc_ctrl_fore                               ; T0: the grid hidden
CTL_TBL equ vdc_ctrl_fill|vdc_ctrl_fore|vdc_ctrl_grid   ; the table
CTL_GON equ vdc_ctrl_fill|vdc_ctrl_grid                 ; a band, cards on
CTL_OFF equ 0                                           ; a band, cards off

; ---- the screen ----
; Characters are 7 rows of 2 lines. Every screen has text line T0 at the top
; and T1 at the bottom (the quads, 16 characters), and eight rows of singles
; between them (fixed.inc).
X_TEXT  equ 16                  ; 16 characters at 8 px, centred
TXT_LEN equ 16
TXT_TAB equ 1+2*TXT_LEN         ; a text table: Y, then 16 x (pointer, control)
    ifndef Y_T0
Y_T0    equ 16                  ; MAME hides characters above Y=14 (NTSC)
    endif

; The table screen. The grid's rows are 24 lines from line 24, its columns 16
; units from X 8 (a character's X register; it draws 5 units right). Seats are
; rows 0-5 (opponents) and the hand rows 6-7 (ours, a card two rows tall).
; A card is a filled cell at columns 1, 3, 5, 7, 9; the name column is X 0-23.
;
; Every row is the same NCOLS singles at the same X: the name (NAMEW), then
; five (rank, suit) pairs. A band writes Y, pointer and control -- never X:
; obj_setup puts X once per screen.
    ifndef NAMEW
NAMEW   equ 1                   ; the name's characters (1 or 2)
    endif
    ifndef NAMEX
NAMEX   equ 8                   ; the name's first X
    endif
    ifndef CARDX
CARDX   equ 0                   ; a rank's X in its cell (the suit is +8)
    endif
NCARDS  equ 5
NCOLS   equ NAMEW+2*NCARDS
ROW_TAB equ 2*NCOLS             ; a row: NCOLS x (pointer, control)
NROWS   equ 8
CARD_COL0 equ 1                 ; grid column of card 0; then every other
    ifndef Y_ROW0
Y_ROW0  equ 30                  ; row 0's characters; row r's is +24r
    endif
ROW_PITCH equ 24
Y_HAND  equ Y_ROW0+6*ROW_PITCH  ; the hand's top row (ranks)
Y_HAND2 equ Y_HAND+ROW_PITCH    ; and its bottom row (suits)
    ifndef Y_T1
Y_T1    equ 228                 ; the table's T1 (MAME draws to line 242)
    endif
    ifndef LEAD
LEAD    equ 1                   ; lines a band starts before its line ends
    endif
    ifndef SEPA
SEPA    equ 3                   ; a row band: the cards go off at this single
    endif
    ifndef SEPAF
SEPAF   equ 1                   ; ... after its pointer (fixed.inc row_write)
    endif
    ifndef SEPB
SEPB    equ 7                   ; ... and on again at this one
    endif
    ifndef SEPBF
SEPBF   equ 1
    endif
    ifndef GRIDNOP
GRIDNOP equ 0                   ; h_grid: nops to put its write in the blank
    endif
X_ROWTXT equ 44                 ; a text screen's rows: 11 characters, centred
    ifndef PADN
PADN    equ 0                   ; spike: pad a row band by 2+2n cycles
    endif

; ---- the stick (input.inc): JOY / JOYEV bits ----
J_UP    equ 01h
J_DOWN  equ 02h
J_LEFT  equ 04h
J_RIGHT equ 08h
J_FIRE  equ 10h

; ---- the keyboard (input.inc): BIOS readkey codes, row x 8 + column ----
K_0     equ 00h                 ; 0-9 are 00h-09h
K_SPACE equ 0Ch
K_QUEST equ 0Dh                 ; ? : help, as H is
K_L     equ 0Eh
K_P     equ 0Fh
K_W     equ 11h
K_E     equ 12h
K_R     equ 13h
K_T     equ 14h
K_U     equ 15h
K_I     equ 16h
K_O     equ 17h
K_Q     equ 18h
K_S     equ 19h
K_D     equ 1Ah
K_F     equ 1Bh
K_G     equ 1Ch
K_H     equ 1Dh
K_J     equ 1Eh
K_K     equ 1Fh
K_A     equ 20h
K_Z     equ 21h
K_X     equ 22h
K_C     equ 23h
K_V     equ 24h
K_B     equ 25h
K_M     equ 26h
K_YES   equ 2Ch
K_NO    equ 2Dh
K_CLR   equ 2Eh
K_ENT   equ 2Fh
K_NONE  equ 0FFh

; ---- sounds (snd.inc): beep ids; from SND_SOFT on they never cut another ----
SND_SELECT equ 0
SND_TURN   equ 1                ; our turn
SND_CHIPS  equ 2                ; a bet, a call, a raise
SND_DEAL   equ 3                ; a street dealt
SND_WIN    equ 4                ; a hand we won
SND_LOSE   equ 5
SND_ERROR  equ 6
SND_SOFT   equ 7
SND_TICK   equ 7                ; our clock, its last seconds
SND_MOVE   equ 8                ; a click

; ---- requests (net.inc): ACT_*, and what a request came to (NETHAVE) ----
ACT_STATE  equ 1
ACT_MOVE   equ 2                ; MOVEC: the move's two letters
ACT_LEAVE  equ 3
ACT_TABLS  equ 4                ; tables?bin=1

NS_IDLE equ 0
NS_CLOS equ 1
NS_OPEN equ 2
NS_STAT equ 3
NS_READ equ 4

NH_FAIL   equ 0
NH_STATE  equ 1                 ; a whole state is in the reply
NH_TABLES equ 2                 ; the table list

; ---- pages, and their farcall entries ----
PG_BOOT  equ 0
E_TABLES equ 1                  ; page 0's: back to the table list
PG_NET   equ 1
E_NBEGIN equ 0
E_NSTEP  equ 1
PG_MAIN  equ 2
PG_STATE equ 3                  ; the table: a state has come ...
PG_TICK  equ 4                  ; ... and every frame
E_STATE  equ 0
E_TITLE  equ 1                  ; page 3's: the boot's title
E_TICK   equ 0
PG_HELP  equ 5
PG_STATE2 equ 6                 ; ... and the rest of it
E_HELP   equ 0
E_NAME   equ 1                  ; page 5's: the name, from the keyboard
HC_TABLES equ 0                 ; the help's contexts
HC_TABLE  equ 1

; ---- the server's binary state (server/util.go serializeResults, ?bin=1):
; strings lowercased and NUL-padded to max+1, u16 little-endian ----
GAME_LASTRESULT equ 0           ; 81
GAME_ROUND      equ 81          ; 0 waiting, 1-4 the streets, 5 showdown
GAME_POT        equ 82          ; u16
GAME_ACTIVE     equ 84          ; the seat to move, us = 0; 0FFh nobody
GAME_MOVETIME   equ 85          ; its seconds left
GAME_VIEWING    equ 86
GAME_NMOVES     equ 87          ; valid moves (5 slots, always)
GAME_MOVES      equ 88          ; 5 x MOVE_SIZE
GAME_COUNT      equ 153         ; playerCount
GAME_SEATS      equ 154         ; SEAT_SIZE each, us first
MOVE_SIZE       equ 13          ; code (3), name (10)
MV_NAME         equ 3
SEAT_SIZE       equ 33
SE_NAME         equ 0           ; 9
SE_STATUS       equ 9           ; PS_*
SE_BET          equ 10          ; u16
SE_MOVE         equ 12          ; 8: its last action
SE_PURSE        equ 20          ; u16
SE_HAND         equ 22          ; 11: 5 x rank, suit; "??" face down
TB_SIZE         equ 36          ; a table: id (9), name (21), players (6)
TB_NAME         equ 9
TB_PLAYERS      equ 30
PS_WAITING equ 0
PS_PLAYING equ 1
PS_FOLDED  equ 2
PS_LEFT    equ 3
ROUND_WAIT equ 0
ROUND_SHOW equ 5
NMOVES     equ 5
MAXSEATS   equ 8
