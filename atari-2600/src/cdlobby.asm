; cdlobby.asm -- bank 0: the cold start and the table list.
;
; Bank 0 by construction: the cold stub in the fixed tail sends the console
; here, because it is the only bank whose number the stub can know before
; anything has run.

        CPU     6502
        INCLUDE "vcs.inc"

PAD3    EQU     $81
SAVSP   EQU     $82

        INCLUDE "fujinet.inc"
        INCLUDE "cddefs.inc"

CDBANK  EQU     BANKLOB
CDHASUI EQU     1
CDHASED EQU     1               ; it writes the joined table's id into a
                                ;   cartridge path buffer
CDHASNET EQU    0               ; BANKNET fetches for it

        INCLUDE "../build/tail.inc"

        ORG     $1000

; ---------------------------------------------------------------------------
LENTRY: lda     #2
        sta     VBLANK
        lda     CDENT
        cmp     #ENCOLD
        beq     LCOLD
        jmp     LWARM

; ---------------------------------------------------------------------------
; The cold start proper. The stub in the tail does only what has to be done
; from an address every bank can see -- the arming pair and the bank switch --
; because the tail is 220 bytes and the shared transport wants 160 of them.
; The rest is here.
LCOLD:  ldx     #$FF
        txs
        lda     #0
LCL1:   sta     $80,x           ; RAM only: $00-$7F is the TIA
        dex
        bne     LCL1
        sta     $80
        lda     #2
        sta     VBLANK
        sta     AUDV0
        sta     AUDV1

; THE CARTRIDGE'S PATH BUFFERS SURVIVE A CONSOLE RESET -- there is no reset
; line on this connector and the cart never sees the switch -- so a client
; that assumes they are empty comes back from a RESET with the last session's
; player name still in its URLs. Empty all four and leave a known one
; selected.
        ldx     #FP_SEL3
LCL2:   txa
        sta     FNRSEL+FH_PATHO
        lda     #FP_RST
        sta     FNRSEL+FH_PATHO
        dex
        cpx     #FP_SEL0-1
        bne     LCL2

        jsr     FNARM           ; harmless twice, and cheap insurance
        jsr     FNCHK
        beq     LGOT
; No cartridge means no character generator either, so there is nothing to say
; it with. A red screen is the whole message.
        lda     #CRED
        sta     COLUBK
        lda     #0
        sta     VBLANK
LHALT:  jmp     LHALT

LGOT:   jsr     DINIT
        jsr     FNCLS
        jsr     CDINKW
        lda     #ENNAME
        sta     CDENT
        lda     #BANKNAM
        jmp     CDGOTO

; ---------------------------------------------------------------------------
; Back from a /tables fetch, or from a redraw.
; THE TEXT PLANES SURVIVE A BANK SWITCH. Coming back here from a table
; without clearing left the game's seat rows, its cards and its move menu
; showing under the lobby's title and footer -- and the lobby paints only the
; rows it uses, so nothing else would ever cover them.
LWARM:  jsr     DINIT
        jsr     FNCLS
        jsr     CDINKW
        lda     #0
        sta     CDSEL
        jsr     LDRAW
        lda     #0
        sta     VBLANK
        jmp     DLOOP

; ---------------------------------------------------------------------------
APPVBL: jsr     INREPT
        sta     CDINP
        and     #IN_UP
        beq     LAV1
        lda     CDSEL
        beq     LAV1
        dec     CDSEL
        jsr     LDRAW
LAV1:   lda     CDINP
        and     #IN_DOWN
        beq     LAV2
        lda     CDSEL
        clc
        adc     #1
        cmp     CDCNT
        bcs     LAV2
        sta     CDSEL
        jsr     LDRAW
LAV2:   lda     CDINP
        and     #IN_RIGHT
        beq     LAV3
        lda     #ENTABLE        ; list again
        sta     CDENT
        lda     #BANKNET
        jmp     CDGOTO
LAV3:   lda     CDINP
        and     #IN_SEL
        beq     LAV4
        lda     #ENNAME         ; change the name
        sta     CDENT
        lda     #BANKNAM
        jmp     CDGOTO
LAV4:   lda     CDINP
        and     #IN_FIRE
        beq     LAV5
        lda     CDCNT
        beq     LAV5
        jmp     LJOIN
LAV5:   rts

; ---------------------------------------------------------------------------
; LJOIN -- take the selected table's id into path buffer 0 and sit down.
;
; This is one of exactly two strings this client copies out of a reply, and it
; goes into the CARTRIDGE, not into console RAM: it has to survive every
; /state poll after it, and each poll repaints the window it came from. Nine
; bytes is more than the 128 can spare and the cartridge holds 256 for free.
;
; The field is fixed-width and space-padded, so it is pushed up to its NUL and
; never through it -- padding in the middle of a URL corrupts the request.
LJOIN:  lda     #PBTABLE
        jsr     FNWSEL
        jsr     FNWRST
        jsr     LTBPTR          ; FNPTRL/H -> the record
        ldy     #TBID
        ldx     #9
LJ1:    lda     (FNPTRL),y
        beq     LJ2
        cmp     #' '
        beq     LJ2
        sta     FNRSEL+FH_PATHC
        iny
        dex
        bne     LJ1
LJ2:    lda     #ENGCOLD
        sta     CDENT
        lda     #BANKGAM
        jmp     CDGOTO

; LTBPTR -- FNPTRL/H points at table CDSEL's record.
;
; The records start at offset ONE, after the count byte, and are 36 bytes
; apart -- so the tenth one is past 256 and this has to be a pointer. Dividing
; the reply length by the stride to get the count looks equivalent to reading
; that first byte and is not: 181 bytes is 1 + 5*36, and the division says 5
; only by accident of rounding.
LTBPTR: lda     #(FNRPLY+1)&$FF
        sta     FNPTRL
        lda     #(FNRPLY+1)>>8
        sta     FNPTRH
        ldx     CDSEL
        beq     LTB2
LTB1:   lda     FNPTRL
        clc
        adc     #TBSTRID
        sta     FNPTRL
        bcc     LTB1A
        inc     FNPTRH
LTB1A:  dex
        bne     LTB1
LTB2:   rts

; ---------------------------------------------------------------------------
; LDRAW -- the whole lobby.
LDRAW:  lda     #RLTITL
        jsr     FNROWA
        ldx     #(LSTITL)&$FF
        ldy     #(LSTITL)>>8
        jsr     FNSETP
        jsr     FNSTRA
        jsr     FNENDR

        lda     CDCNT
        bne     LD1
; An empty lobby is a real answer, not a failure. Say so and offer the retry.
        lda     #RLIST0
        jsr     FNROWA
        ldx     #(LSNONE)&$FF
        ldy     #(LSNONE)>>8
        jsr     FNSETP
        jsr     FNSTRA
        jsr     FNENDR
        lda     #$FF
        sta     CDBAR
        jmp     LDFOOT

LD1:    lda     #0
        sta     CDIDX
LD2:    lda     CDIDX
        clc
        adc     #RLIST0
        jsr     FNROWA
        lda     CDIDX
        cmp     CDCNT
        bcs     LDBLK
        ; the row is the table's name, eleven of the twenty-one bytes it sends
        jsr     LTBPTR2
        ldy     #TBNAME
        ldx     #FNTCOL
LD3:    lda     (FNPTRL),y
        beq     LD4
        sta     FNRSEL+FH_TCHR
        iny
        dex
        bne     LD3
        jmp     LD5
LD4:    lda     #' '
        sta     FNRSEL+FH_TCHR
        dex
        bne     LD4
LD5:    jsr     FNENDR
        jmp     LDNXT
LDBLK:  lda     #FNTCOL
        jsr     FNSPC
        jsr     FNENDR
LDNXT:  inc     CDIDX
        lda     CDIDX
        cmp     #MAXTBL
        bne     LD2

; The cursor is the same red bar the move menu uses. There is no inverse video
; and no per-cell colour on this machine, so a whole-row band is the only
; highlight there is -- and having one highlight mean one thing everywhere is
; better than inventing a second.
        lda     CDSEL
        clc
        adc     #RLIST0
        sta     CDBAR

; The seat count the server pre-formats as "cur / max". It is a literal
; string, not two numbers -- only the JSON form splits it -- so it is printed
; verbatim.
        lda     #RLSEAT
        jsr     FNROWA
        jsr     LTBPTR
        ldy     #TBPLYRS
        ldx     #6
LD6:    lda     (FNPTRL),y
        beq     LD7
        sta     FNRSEL+FH_TCHR
        iny
        dex
        bne     LD6
LD7:    jsr     FNENDR

LDFOOT: lda     #RLFOOT
        jsr     FNROWA
        ldx     #(LSFOOT)&$FF
        ldy     #(LSFOOT)>>8
        jsr     FNSETP
        jsr     FNSTRA
        jmp     FNENDR

; LTBPTR2 -- the same pointer, for the row being drawn rather than the one
; selected.
LTBPTR2: lda    #(FNRPLY+1)&$FF
        sta     FNPTRL
        lda     #(FNRPLY+1)>>8
        sta     FNPTRH
        ldx     CDIDX
        beq     LTC2
LTC1:   lda     FNPTRL
        clc
        adc     #TBSTRID
        sta     FNPTRL
        bcc     LTC1A
        inc     FNPTRH
LTC1A:  dex
        bne     LTC1
LTC2:   rts

RLTITL  EQU     0
RLIST0  EQU     3
RLSEAT  EQU     15
RLFOOT  EQU     19

LSTITL: DB      "5 CARD STUD",0
LSNONE: DB      "NO TABLES",0
LSFOOT: DB      "FIRE TO SIT",0

; The lobby is a list, so its rows are plain: no seat bands to separate.
CDBGT:  DB      CBLACK,CBLACK,CBLACK,CBLACK,CBLACK,CBLACK
        DB      CBLACK,CBLACK,CBLACK,CBLACK,CBLACK,CBLACK
        DB      CBLACK,CBLACK,CBLACK,CBLACK,CBLACK,CBLACK
        DB      CBLACK,CBLACK,CBLACK
        DB      CGREEN                  ; row 21: back to the felt

        INCLUDE "cdlib.inc"
        INCLUDE "state.inc"
        INCLUDE "cdisp.inc"

        END
