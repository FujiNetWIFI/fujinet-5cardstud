; cdnet.asm -- bank 2: one request, and back.
;
; This bank exists for a size reason and it is not a small one. net.inc and
; url.inc are about 550 bytes together, and every bank carries its own copy of
; everything it calls; the game bank needs the display kernel, the renderer
; and the move menu and came in 552 bytes over with the network in it.
;
; A poll happens every ninety frames, so paying a bank switch for one costs
; nothing measurable.
;
; IT DOES CARRY THE DISPLAY KERNEL, though it composes nothing. A /state poll
; measured three to four frames of dead screen every ninety-four -- a flash,
; once a second and a half -- and it was not the network: two of those frames
; were the settle loop's own deliberate delay, spun blind in a bank that could
; not draw, and the rest was the transport waiting on ACKSEQ. Neither wait
; emits a VSYNC, so the picture did not go black so much as stop being a
; picture. net.inc now spends both of them in DFRAME instead, which redraws
; the table out of the text planes the cartridge is still holding.
;
; So CDHASUI is still 0 -- there is nothing to COMPOSE here, and the renderer
; and most of cdlib stay guarded out -- but cdisp.inc and a row-colour table
; are in, and APPVBL is an RTS because a frame drawn from inside a transaction
; must not start another one.
;
; The table below is the GAME screen's, and that is the whole reason the
; drawing path is gated on CDREQ: this bank is entered from the lobby too, and
; a bank has room for one CDBGT, not two.

        CPU     6502
        INCLUDE "vcs.inc"

PAD3    EQU     $81
SAVSP   EQU     $82

        INCLUDE "fujinet.inc"
        INCLUDE "cddefs.inc"

CDBANK  EQU     BANKNET
CDHASUI EQU     0               ; it draws nothing and reads no input
CDHASED EQU     0
CDHASNET EQU    1               ; it is the only bank that issues requests

        INCLUDE "../build/tail.inc"

        ORG     $1000

; ---------------------------------------------------------------------------
; Entered with CDENT saying which request, and leaving with CDENT saying where
; to carry on. Blank first: a game poll's first paced wait unblanks again a
; frame later, and the lobby's one-off fetches stay blanked all the way
; through, which is invisible because they happen on a screen change.
NENTRY: lda     #2
        sta     VBLANK

        lda     CDENT
        cmp     #ENTABLE
        beq     NTABLE
        cmp     #ENLEAVE
        beq     NLEAVE

; ---------------------------------------------------------------------------
; A game poll. A STAGED MOVE RIDES IT: /move/XX replaces /state and its reply
; IS the next state, so there is no separate submit anywhere in this client.
        lda     CDMOVE
        beq     NPOLL
        lda     #RQMOVE
        jmp     NGO
NPOLL:  lda     #RQSTATE
NGO:    jsr     APICALL
        sta     CDERR2
        lda     #0
        sta     CDMOVE          ; sent, or abandoned; either way not re-sent
        lda     CDERR2
        beq     NGOOD
; A failure leaves the reply window alone, so the table on screen is still the
; last good one. Back off and let the game bank say so on the bottom bar.
        lda     #FAILFRM
        sta     CDPOLL
        jmp     NBACKG
NGOOD:  lda     #POLLFRM
        sta     CDPOLL
        lda     #0
        sta     CDERR
NBACKG: lda     #ENGAME
        sta     CDENT
        lda     #BANKGAM
        jmp     CDGOTO

; ---------------------------------------------------------------------------
; The lobby's table list.
NTABLE: lda     #RQTABLE
        jsr     APICALL
        beq     NTOK
        lda     #0
        sta     CDCNT           ; no tables to show
        jmp     NBACKL
NTOK:   lda     #0
        sta     CDERR
NBACKL: lda     #ENLOBBY
        sta     CDENT
        lda     #BANKLOB
        jmp     CDGOTO

; ---------------------------------------------------------------------------
; Leaving. The reply is a Game and is thrown away; what matters is that the
; server frees the seat.
NLEAVE: lda     #RQLEAVE
        jsr     APICALL
        lda     #ENCOLD
        sta     CDENT
        lda     #BANKLOB
        jmp     CDGOTO

; ---------------------------------------------------------------------------
; APPVBL -- nothing. DFRAME calls this every frame, and every frame this bank
; draws is one drawn from INSIDE a transaction: polling for another here would
; re-enter APICALL through its own settle loop.
APPVBL: rts

; ---------------------------------------------------------------------------
; CDBGT -- the background of each row. A copy of cdgame.asm's, because the
; only screen this bank ever draws is the table: NFRAME draws for RQSTATE and
; RQMOVE, which are the two requests the game bank issues, and spins blind for
; the lobby's RQTABLE and RQLEAVE. Keep the two tables identical.
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
        INCLUDE "net.inc"
        INCLUDE "url.inc"
        INCLUDE "state.inc"
        INCLUDE "cdisp.inc"

        END
