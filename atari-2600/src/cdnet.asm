; cdnet.asm -- bank 2: one request, and back.
;
; This bank exists for a size reason and it is not a small one. net.inc and
; url.inc are about 550 bytes together, and every bank carries its own copy of
; everything it calls; the game bank needs the display kernel, the renderer
; and the move menu and came in 552 bytes over with the network in it.
;
; A poll happens every ninety frames, so paying a bank switch for one costs
; nothing measurable. And a bank with no display kernel does not mind that the
; screen is blanked while it talks -- the screen was blanked for the
; transaction anyway, because a round trip takes seconds and no 2600 client in
; this family pretends otherwise.
;
; There is nothing to draw here, so CDHASUI is 0 and most of cdlib and all of
; the renderer are guarded out. What is left is the transport (in the fixed
; tail), the N: round trip, the URL streamer and the length check.

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
; to carry on. The screen stays blanked throughout: there is no kernel in this
; bank to run it.
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

        INCLUDE "cdlib.inc"
        INCLUDE "net.inc"
        INCLUDE "url.inc"
        INCLUDE "state.inc"

        END
