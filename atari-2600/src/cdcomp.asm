; cdcomp.asm -- bank 5: compose the whole table, in one frame of exactly the
; right length.
;
; THIS BANK EXISTS FOR A TIMING REASON, not a size one, which makes it the odd
; one out. Composing twenty-one rows is about 12,900 cycles. The display
; kernel's vblank -- the only blanked time a frame has, since DPADA and DPADB
; are the visible green bands -- is 2,812. So GWARM's recompose after a poll
; could not run inside a frame, and running it between frames made that frame
; about 430 scanlines instead of 262. A frame of a different length is a
; picture in a different place, so the table jumped, once every poll.
;
; There was no room to fix it in bank 1: cdgame had thirty-two bytes left.
; Here there are two thousand, and what they buy is a frame skeleton that is
; 262 scanlines whatever the compose costs -- three of VSYNC, a timed window
; wide enough to compose in, and a counted pad to make up the difference. The
; screen is blanked for it, so a poll costs one black frame and NO movement.
;
; It carries a copy of render.inc rather than taking it away from bank 1,
; because bank 1 still composes the small things itself: the move bar on a
; cursor key is a couple of rows and fits the vblank budget with room to
; spare. Only the whole table does not. Every bank in this client carries its
; own copy of what it calls; this is that rule, not an exception to it.
;
; NO DISPLAY KERNEL. This bank never shows a picture -- it blanks, composes
; and leaves -- so cdisp.inc and its row-colour table stay out, which is most
; of what a bank this size would otherwise spend.

        CPU     6502
        INCLUDE "vcs.inc"

PAD3    EQU     $81
SAVSP   EQU     $82

        INCLUDE "fujinet.inc"
        INCLUDE "cddefs.inc"

CDBANK  EQU     BANKCMP
CDHASUI EQU     1               ; it composes, so it needs the text primitives
CDHASED EQU     0               ; but not the editor
CDHASNET EQU    0               ; and it issues no requests

        INCLUDE "../build/tail.inc"

        ORG     $1000

; ---------------------------------------------------------------------------
; The frame this bank is for.
;
; CWTIM is a T1024T window -- 1024 cycles a tick, so seventeen of them is
; 17,408 cycles or about 229 scanlines, comfortably more than a compose needs
; and comfortably less than a frame. CWPAD is the rest of the 262 in whole
; scanlines. Both were histogrammed with emu/blank.lua rather than derived:
; the timer's granularity is 13.5 scanlines and the WSYNC after it rounds up,
; so the arithmetic only gets you close.
;
; TIMINT, not INTIM, for the reason cdisp.inc gives: past zero the timer
; free-runs at a tick a cycle and a polling loop walks straight over the one
; cycle it reads zero on. The latch is what makes an overrun cost only the
; overrun -- and an overrun here is a compose that grew, which is the one
; thing that would put the jump back.
CWTIM   EQU     17
CWPAD   EQU     29

CENTRY: lda     #2
        sta     VBLANK
        lda     #2
        sta     VSYNC
        sta     WSYNC
        sta     WSYNC
        sta     WSYNC
        lda     #0
        sta     VSYNC
        lda     #CWTIM
        sta     T1024T

; What to compose. ENDRAW is the whole table, after a poll; ENDRSEA is the
; eight seats, which is what a SELECT press changes and is still five times
; what a frame's vblank could hold.
        lda     CDENT
        cmp     #ENDRSEA
        bne     CW0
        jsr     GSEATS
        jmp     CWW
CW0:    jsr     GRENDER

CWW:    bit     TIMINT
        bpl     CWW
        sta     WSYNC
        ldx     #CWPAD
CW2:    sta     WSYNC
        dex
        bne     CW2

; Back to the table, composed. ENGRUN and not ENGAME: ENGAME means "back from
; a poll, so recompose", which is what sent us here.
        lda     #ENGRUN
        sta     CDENT
        lda     #BANKGAM
        jmp     CDGOTO

; ---------------------------------------------------------------------------
; APPVBL is the display kernel's hook and there is no kernel here, but
; cdlib.inc's guards do not know that. An RTS costs one byte and keeps the
; includes honest.
APPVBL: rts

        INCLUDE "cdlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "state.inc"
        INCLUDE "render.inc"

        END
