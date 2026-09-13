; cdmenu.asm -- bank 3: the in-game menu and the help screen.
;
; Reached by the RESET switch at the table, or by pushing the stick LEFT. The
; 2600 has one button and four directions and no keypad, and the move menu
; already owns up, down and fire -- so left is what is left of the stick, and
; RESET is the only labelled button going spare. It is a SWITCH on this
; console, SWCHB bit 0, which the program reads: it restarts nothing, and what
; it means is the client's to choose. Both gestures also back OUT of here, for
; the same reason a door opens both ways.

        CPU     6502
        INCLUDE "vcs.inc"

PAD3    EQU     $81
SAVSP   EQU     $82

        INCLUDE "fujinet.inc"
        INCLUDE "cddefs.inc"

CDBANK  EQU     BANKMNU
CDHASUI EQU     1
CDHASED EQU     0
CDHASNET EQU    0               ; /leave is BANKNET's, like every request

        INCLUDE "../build/tail.inc"

RMTITL  EQU     0
RMITEM0 EQU     3
NMITEMS EQU     3
RMHELP0 EQU     9

MIRES   EQU     0               ; resume
MIHELP  EQU     1               ; how to play
MILEAVE EQU     2               ; leave the table

        ORG     $1000

MENTRY: lda     #2
        sta     VBLANK
        jsr     DINIT
        jsr     FNCLS
        jsr     CDINKW
        lda     #0
        sta     CDSEL
        sta     CDMHELP
        jsr     MDRAW
        lda     #0
        sta     VBLANK
        jmp     DLOOP

APPVBL: jsr     INREPT
        sta     CDINP

; The help screen swallows everything but a button, which closes it.
        lda     CDMHELP
        beq     MAV0
        lda     CDINP
        and     #IN_FIRE|IN_LEFT|IN_RST
        beq     MAVR
        lda     #0
        sta     CDMHELP
        jsr     FNCLS
        jmp     MDRAW

MAV0:   lda     CDINP
        and     #IN_UP
        beq     MAV1
        lda     CDSEL
        beq     MAV1
        dec     CDSEL
        jsr     MDRAW
MAV1:   lda     CDINP
        and     #IN_DOWN
        beq     MAV2
        lda     CDSEL
        cmp     #NMITEMS-1
        bcs     MAV2
        inc     CDSEL
        jsr     MDRAW
MAV2:   lda     CDINP
        and     #IN_LEFT|IN_RST ; either one again backs out, as it came in
        bne     MRESUME
        lda     CDINP
        and     #IN_FIRE
        beq     MAVR
        lda     CDSEL
        cmp     #MIHELP
        beq     MHELP
        cmp     #MILEAVE
        beq     MLEAVE
MRESUME: lda    #ENGAME
        sta     CDENT
        lda     #0
        sta     CDERR           ; resuming is not a failed poll
        lda     #BANKGAM
        jmp     CDGOTO
MAVR:   rts

MHELP:  lda     #1
        sta     CDMHELP
        lda     #$FF
        sta     CDBAR
        jsr     FNCLS
        jmp     MDRAW

; Leaving is a request, and requests live in BANKNET. It sends /leave and goes
; on to the lobby whatever the server says: the seat is given up either way,
; and a client that refused to leave because the link hiccuped would be stuck.
MLEAVE: lda     #ENLEAVE
        sta     CDENT
        lda     #BANKNET
        jmp     CDGOTO

; ---------------------------------------------------------------------------
MDRAW:  lda     CDMHELP
        bne     MDHELP

        lda     #RMTITL
        jsr     FNROWA
        ldx     #(MSTITL)&$FF
        ldy     #(MSTITL)>>8
        jsr     FNSETP
        jsr     FNSTRA
        jsr     FNENDR

        lda     #0
        sta     CDIDX
MD1:    lda     CDIDX
        clc
        adc     #RMITEM0
        jsr     FNROWA
        lda     CDIDX
        asl     a
        asl     a
        sta     CDTMP           ; twelve bytes an item
        asl     a
        clc
        adc     CDTMP
        tax
        ldy     #FNTCOL
MD2:    lda     MSITEMS,x
        sta     FNRSEL+FH_TCHR
        inx
        dey
        bne     MD2
        jsr     FNENDR
        inc     CDIDX
        lda     CDIDX
        cmp     #NMITEMS
        bne     MD1

        lda     CDSEL
        clc
        adc     #RMITEM0
        sta     CDBAR
        rts

; The help screen: a list of rows in ROM, terminated by a zero length.
MDHELP: lda     #0
        sta     CDIDX
MDH1:   lda     CDIDX
        clc
        adc     #RMHELP0-RMHELP0
        sta     CDTMP
        lda     CDIDX
        asl     a
        asl     a
        sta     CDNUM0
        asl     a
        clc
        adc     CDNUM0          ; twelve bytes a row
        tax
        lda     MSHELP,x
        beq     MDH2            ; a zero first byte ends the list
        lda     CDIDX
        jsr     FNROWA
        ldy     #FNTCOL
MDH1A:  lda     MSHELP,x
        sta     FNRSEL+FH_TCHR
        inx
        dey
        bne     MDH1A
        jsr     FNENDR
        inc     CDIDX
        lda     CDIDX
        cmp     #FNTROW
        bne     MDH1
MDH2:   rts

MSTITL: DB      "TABLE MENU",0
MSITEMS: DB     "RESUME      "
        DB      "HOW TO PLAY "
        DB      "LEAVE TABLE "
; Twelve columns a row, padded, terminated by a row that starts with a NUL.
MSHELP: DB      "HOW TO PLAY "
        DB      "            "
        DB      "5 CARD STUD "
        DB      "1 HOLE CARD "
        DB      "4 FACE UP   "
        DB      "            "
        DB      "UP DOWN PICK"
        DB      "FIRE  SENDS "
        DB      "            "
        DB      "HOLD SELECT "
        DB      "SHOWS PURSES"
        DB      "            "
        DB      "LEFT   MENU "
        DB      "RIGHT  POLL "
        DB      "            "
        DB      "FIRE CLOSES "
        DB      0

CDBGT:  DB      CBLACK,CBLACK,CBLACK,CBLACK,CBLACK,CBLACK
        DB      CBLACK,CBLACK,CBLACK,CBLACK,CBLACK,CBLACK
        DB      CBLACK,CBLACK,CBLACK,CBLACK,CBLACK,CBLACK
        DB      CBLACK,CBLACK,CBLACK
        DB      CGREEN

        INCLUDE "cdlib.inc"
        INCLUDE "state.inc"
        INCLUDE "cdisp.inc"

        END
