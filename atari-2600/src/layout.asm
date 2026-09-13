; layout.asm -- the screen, with no network at all.
;
; A flat 4K image that composes one fixed table and runs the display kernel
; forever. It exists because the kernel is the one component of this port with
; no template anywhere: the seam-line colour loop, the green playfield frame,
; the two-row seat geometry and the red move bar all have to be right before
; any of them is worth debugging over a socket.
;
; What it cannot show is the card bed. FB_CARD's source is an offset into the
; reply window, and the console has no way to write that window -- so real
; card art needs a real reply. It is checked bit-for-bit instead by
; firmware/host_test/test_render.c, and on screen by the live harness.
;
;   ./build.sh layout && SLOT=fujinet ./run.sh layout shot

        CPU     6502
        INCLUDE "vcs.inc"

; The display kernel's two cells, which have to exist before it is included.
PAD3    EQU     $81             ; its 3-cycle pad target
SAVSP   EQU     $82             ; the stack pointer, parked across the kernel

        INCLUDE "fujinet.inc"
        INCLUDE "cddefs.inc"
CDBANK  EQU     BANKGAM
CDHASUI EQU     1
CDHASED EQU     0
CDHASNET EQU    0
; No build/tail.inc here: this ROM assembles the tail body itself at the
; bottom of the file, so the labels are the real thing and not equates
; pointing at it. Including both is a double definition, which is a nicer
; failure than the alternative -- including only the equates is what made
; every jsr into the shared transport land on p2bin's filler.

        ORG     $1000

START:  sei
        cld
        ldx     #$FF
        txs
        lda     #0
LCLR:   sta     $00,x           ; $00-$7F is the TIA, $80-$FF is RAM
        dex
        bne     LCLR
        sta     $00

        lda     #2              ; blanked until there is something to show
        sta     VBLANK

        jsr     FNARM
        jsr     FNCHK
        beq     LGOT
        ; No cartridge: there is no character generator either, so there is
        ; nothing to say it with. A red screen is the whole message.
        lda     #CRED
        sta     COLUBK
LHALT:  jmp     LHALT

LGOT:   jsr     DINIT
        jsr     LDRAW
        lda     #0
        sta     VBLANK
        jmp     DLOOP

; APPVBL -- the kernel's per-frame hook. Walking the red bar down the four
; menu rows and back proves the seam line reprogrammes COLUBK cleanly on every
; row, which a still screenshot cannot.
APPVBL: lda     CDFRAME
        and     #$3F
        bne     APV1
        inc     CDSEL
        lda     CDSEL
        and     #$03
        sta     CDSEL
        clc
        adc     #RBAR0
        sta     CDBAR
APV1:   rts

; ---------------------------------------------------------------------------
; LDRAW -- compose the whole screen once.
LDRAW:  jsr     FNCLS

; Row 0: the pot and your purse. No spaces between them -- twelve columns is
; exactly 'P' + five digits + '$' + five digits, and the sigils separate the
; two numbers better than a gap would.
        lda     #RHEAD
        jsr     FNROWA
        lda     #'P'
        jsr     FNCHR
        lda     #(1250)&$FF
        sta     CDNUM0
        lda     #(1250)>>8
        sta     CDNUM1
        lda     #5
        jsr     CDDEC
        lda     #'$'
        jsr     FNCHR
        lda     #(420)&$FF
        sta     CDNUM0
        lda     #(420)>>8
        sta     CDNUM1
        lda     #5
        jsr     CDDEC
        jsr     FNENDR

; The eight seats. Columns 0-7 are the card bed and stay blank here; the name
; goes in 8-11 of the upper row and the value in 8-11 of the lower.
        ldx     #0
LSEAT:  stx     CDIDX
        txa
        asl     a               ; two rows per seat
        clc
        adc     #RSEAT0
        pha                     ; the upper row
        jsr     FNROWA
        lda     #CDCOLTX
        jsr     FNSPC
        ldx     CDIDX
        jsr     LNAME
        jsr     FNENDR
        pla
        clc
        adc     #1              ; the lower row
        jsr     FNROWA
        lda     #CDCOLTX
        jsr     FNSPC
        ldx     CDIDX
        lda     LVALL,x
        sta     CDNUM0
        lda     LVALH,x
        sta     CDNUM1
        jsr     CDCL4
        lda     #4
        jsr     CDDEC
        jsr     FNENDR
        ; The seat's colour, decided here and read by the kernel's seam line.
        ldx     CDIDX
        lda     LCOLT,x
        sta     CDSCOL,x
        inx
        cpx     #MAXPLYR
        bne     LSEAT

; The bottom bar: four move rows, one per row so the red bar can be the whole
; width of one.
        ldx     #0
LBAR:   stx     CDIDX
        txa
        clc
        adc     #RBAR0
        jsr     FNROWA
        lda     CDIDX
        asl     a
        asl     a
        asl     a               ; eight bytes a name
        tax
LBAR1:  lda     LMOVES,x
        beq     LBAR2
        jsr     FNCHR
        inx
        jmp     LBAR1
LBAR2:  jsr     FNENDR
        ldx     CDIDX
        inx
        cpx     #NBARROW
        bne     LBAR
        rts

; LNAME -- append seat X's four-character name.
LNAME:  txa
        asl     a
        asl     a               ; four bytes a name
        tax
        ldy     #4
LNAM1:  lda     LNAMES,x
        jsr     FNCHR
        inx
        dey
        bne     LNAM1
        rts

; ---------------------------------------------------------------------------
; The mock table. Four-character names because that is the field: the server
; sends nine and the Intellivision port truncates to four for the same reason.
LNAMES: DB      "THOM"          ; seat 0 is always you
        DB      "ADA "
        DB      "BOT1"
        DB      "KAY "
        DB      "REX "
        DB      "MAE "
        DB      "IVY "
        DB      "ZED "
LVALL:  DB      (420)&$FF,(35)&$FF,(1200)&$FF,(0)&$FF
        DB      (75)&$FF,(9999)&$FF,(12345)&$FF,(8)&$FF
LVALH:  DB      (420)>>8,(35)>>8,(1200)>>8,(0)>>8
        DB      (75)>>8,(9999)>>8,(12345)>>8,(8)>>8
; One of each, so every branch of the seam line's colour pick is on screen:
; you, the seat to act, two that folded, and the rest still in.
LCOLT:  DB      CWHITE,CGOLD,CGREY,CDIM,CGREY,CGREY,CDIM,CGREY
LMOVES: DB      "FOLD",0,0,0,0
        DB      "CHECK",0,0,0
        DB      "BET 50",0,0
        DB      "ALL-IN",0,0

; ---------------------------------------------------------------------------
; CDBGT -- the background of each row, indexed 0-21.
;
; Twenty-TWO entries: the seam line at the bottom of row 20 programmes row 21,
; which does not exist, and making that entry felt is what turns the handover
; to the bottom band into a colour the kernel was going to write anyway.
;
; Seats alternate black and near-black in pairs. Eight two-row seats with no
; gap between them run together otherwise, and there is no row to spare for a
; rule.
CDBGT:  DB      CBLACK                          ; 0   the pot and your purse
        DB      CBLACK,CBLACK                   ; 1-2   seat 0 -- you
        DB      CDARK,CDARK                     ; 3-4   seat 1
        DB      CBLACK,CBLACK                   ; 5-6   seat 2
        DB      CDARK,CDARK                     ; 7-8   seat 3
        DB      CBLACK,CBLACK                   ; 9-10  seat 4
        DB      CDARK,CDARK                     ; 11-12 seat 5
        DB      CBLACK,CBLACK                   ; 13-14 seat 6
        DB      CDARK,CDARK                     ; 15-16 seat 7
        DB      CBLACK,CBLACK,CBLACK,CBLACK     ; 17-20 the bottom bar
        DB      CGREEN                          ; 21    back to the felt

        INCLUDE "cdlib.inc"
        INCLUDE "cdisp.inc"

; The fixed half, from the same source the real client's tail is built from,
; so the addresses build/tail.inc hands out are true here too. Its CDCOLD is
; the reset vector and it arrives at $1000 -- which in a flat image is START.
        INCLUDE "cdtailbody.inc"

        END
