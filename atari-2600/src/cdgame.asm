; cdgame.asm -- bank 1: the table.
;
; Poll /state, render eight seats out of the reply window, take a bet. The
; whole screen is composed from the cartridge's own copy of the server's
; reply: nothing in here holds a player, a card or a name.

        CPU     6502
        INCLUDE "vcs.inc"

PAD3    EQU     $81             ; the display kernel's 3-cycle pad target
SAVSP   EQU     $82             ; the stack pointer, parked across the kernel

        INCLUDE "fujinet.inc"
        INCLUDE "cddefs.inc"

; AFTER cddefs.inc, not before: url.inc's `IF CDBANK=BANKGAM` has to be
; evaluatable in the assembler's FIRST pass, and a forward reference to
; BANKGAM is not. Setting it before the include assembles every bank's copy of
; url.inc with all three request paths in it, which is how you find out.
CDBANK  EQU     BANKGAM
CDHASUI EQU     1               ; it draws and reads input
CDHASED EQU     0               ; it edits no path buffer
CDHASNET EQU    0               ; and it issues no requests: BANKNET does

; The shared transport's addresses, read back out of the tail's own listing by
; tools/mktail.py. Equates, so before the ORG like every other equate.
        INCLUDE "../build/tail.inc"

        ORG     $1000

; ---------------------------------------------------------------------------
; Entered from the trampoline in the fixed tail. CDENT says why.
;
; A bank switch is a jump, so there is no return address: arriving here from a
; poll is a fresh entry and has to be told apart from arriving here from the
; lobby. That is what CDENT is for, and it is the Odyssey 2 port's X_RET by
; another name.
GENTRY: lda     #2
        sta     VBLANK
        lda     CDENT
        cmp     #ENGRUN         ; BANKCMP has composed it: nothing to do but
        beq     GRUN            ;   draw, and DINIT must NOT run again -- it
                                ;   would clear the bar GBAR just set
        cmp     #ENGCOLD
        bne     GWARM

; A new table.
        lda     #1
        sta     CDREDRW
        sta     CDPOLL          ; poll on the very first frame
        lda     #0
        sta     CDMOVE
        sta     CDSEL
        sta     CDMVTOP
        sta     CDLRPG
        sta     CDCLK
        sta     CDSELHD
; The banner's baseline is armed SILENTLY, so a hand that finished before we
; sat down is never announced as if we had seen it. Zero is the right
; baseline: a NUL-padded lastResult sums to zero and matches.
        sta     CDLRSUM
        lda     #$FF
        sta     CDBAR           ; no bar until a menu asks for one
        sta     CDPRVAC
        sta     CDPRVRD
        jsr     DINIT
        jsr     FNCLS
        jmp     GRUN

; Back from a poll. BANKNET has already set CDPOLL and either cleared CDERR or
; left the failure in it.
GWARM:  jsr     DINIT
        lda     CDERR
        beq     GW1
        jsr     GFAIL
        jmp     GRUN
; The recompose is twenty-one rows and about 12,900 cycles, which is four
; times the vblank budget and cannot happen inside a frame. BANKCMP does it in
; a frame of its own that is exactly 262 scanlines long, so the picture stays
; where it is. See cdcomp.asm.
GW1:    lda     #ENDRAW
        sta     CDENT
        lda     #BANKCMP
        jmp     CDGOTO
; The run loop, and it is DLOOP with one thing added: the poll is started
; HERE, between frames, and not from inside APPVBL.
;
; A bank switch taken from the hook abandons the frame in its vblank, before
; it has drawn a single line, so that frame came out black -- one of the two
; blinks a poll used to cost. APPVBL now only counts CDPOLL down and leaves it
; at zero; the switch waits for DFRAME to finish the picture first.
GRUN:   jsr     DFRAME
        lda     CDPOLL
        bne     GRUN
; Time to poll. That is a whole other bank: net.inc and url.inc are 550 bytes
; and this one has a display kernel, a renderer and a move menu to fit.
        lda     #ENFETCH
        sta     CDENT
        lda     #BANKNET
        jmp     CDGOTO

; ---------------------------------------------------------------------------
; APPVBL -- the kernel's per-frame hook, called with the screen blanked.
;
; A transaction takes far longer than a frame, but it no longer stalls the
; picture: BANKNET waits through DFRAME, so the table stays on screen for the
; whole round trip. What is still a stall is GWARM's recompose on the way back
; -- one frame of composing twenty-one rows, which is work and not waiting.
APPVBL: jsr     SNDTICK
        jsr     INREPT
        sta     CDINP

; SELECT is a LEVEL, not an edge: hold it and every seat's value field shows
; that seat's purse instead of its bet. Only the CHANGE costs a redraw.
        lda     INCUR
        and     #IN_SEL
        beq     APV1
        lda     #1
APV1:   cmp     CDSELHD
        beq     APV2
        sta     CDSELHD
; Sixteen rows, which is five times the vblank budget: composing it here would
; push the picture down the screen by everything it cost. BANKCMP, like the
; recompose after a poll.
        lda     #ENDRSEA
        sta     CDENT
        lda     #BANKCMP
        jmp     CDGOTO

APV2:   lda     CDINP
        and     #IN_LEFT
        beq     APV2R
        lda     #BANKMNU        ; the in-game menu: resume, or leave
        jmp     CDGOTO

; The RESET switch is a SWITCH on this console -- SWCHB bit 0, which the
; program reads -- and not a CPU reset: nothing reboots unless the client makes
; it. Leaving a seat properly is the menu's LEAVE; this is the blunt way out.
APV2R:  lda     CDINP
        and     #IN_RST
        beq     APV3
        lda     #ENCOLD
        sta     CDENT
        lda     #BANKLOB
        jmp     CDGOTO

APV3:   lda     CDINP
        and     #IN_RIGHT
        beq     APV4
        lda     #1
        sta     CDPOLL          ; poll now

APV4:   jsr     GMOVEUI

; The banner's page timer. It runs off the frame clock rather than the poll
; clock so a page is a page whatever the server is doing.
        lda     CDLRPG
        beq     APV6
        dec     CDLRHLD
        bne     APV6
        jsr     GLRNEXT

; The poll clock. It stops AT zero rather than switching banks here -- GRUN
; does that once the frame this is running inside has been drawn -- so the
; test has to come before the decrement, or a due poll would wrap CDPOLL back
; to 255 on the very next frame.
APV6:   lda     CDPOLL
        beq     APV7
        dec     CDPOLL
APV7:   rts

; ---------------------------------------------------------------------------
; GMOVEUI -- the cursor, the clock, and the submit.
;
; The countdown is a LOCAL variable and not the reply's moveTime byte. The
; reply window is the cartridge's, which is ROM from here: counting down in
; place would go nowhere and the clock would never expire. The ColecoVision
; port found the same thing.
;
; A TIMEOUT SUBMITS THE HIGHLIGHT, not a fold, and the highlight defaults to
; index 1 -- never FOLD. Both are the shared C core's behaviour.
GMOVEUI: jsr   GMYTRN
        beq     GMU1
        lda     #0
        sta     CDCLK
        rts
GMU1:   lda     CDACT
        cmp     CDPRVAC
        bne     GMU1S           ; a fresh turn: seed the cursor and the clock
; Still our turn, and the clock has stopped with nothing on its way. Either
; the move we sent was not one the server would take, or the turn edge fell
; between two polls. Start the clock again rather than sitting on a dead one,
; which is a table that never moves and no way for the player to tell why.
        lda     CDCLK
        bne     GMU2
        lda     CDMOVE
        bne     GMU2
GMU1S:  lda     CDACT
        sta     CDPRVAC
        lda     #1
        cmp     CDNMV           ; never default to FOLD, unless FOLD is all
        bcc     GMU1A           ;   there is
        lda     #0
GMU1A:  sta     CDSEL
        lda     FNRPLY+GMOVET
        sta     CDCLK
        lda     #60
        sta     CDTICK
        lda     #SNDTURN
        jsr     SNDFIRE
        jsr     GBAR

GMU2:   lda     CDINP           ; the newly-pressed mask, from APPVBL
        and     #IN_UP
        beq     GMU3
        lda     CDSEL
        beq     GMU3
        dec     CDSEL
        jsr     GBAR
GMU3:   lda     CDINP
        and     #IN_DOWN
        beq     GMU4
        lda     CDSEL
        clc
        adc     #1
        cmp     CDNMV
        bcs     GMU4
        sta     CDSEL
        jsr     GBAR
GMU4:   lda     CDINP
        and     #IN_FIRE
        bne     GMUSUB

; the clock, ticking locally at 1Hz
        lda     CDCLK
        beq     GMU5
        dec     CDTICK
        bne     GMU5
        lda     #60
        sta     CDTICK
        dec     CDCLK
        bne     GMU4A
        jmp     GMUSUB          ; expired: send the highlight
GMU4A:  jsr     GBAR            ; repaint just for the two clock digits
GMU5:   rts

; GMUSUB -- stage the selected move. It goes out on the next poll, which is
; forced to be the very next frame.
GMUSUB: lda     #SNDSENT
        jsr     SNDFIRE
        lda     CDSEL
        jsr     GMVCOD
        lda     #0
        sta     CDCLK
        lda     #1
        sta     CDPOLL
        rts

; GMVCOD -- copy move A's two-character code out of the reply into CDMOVE.
;
; This is the one thing that MUST be copied: the code is read now and sent on
; the next request, and that request repaints the window it was read from.
GMVCOD: jsr     GMVOFF          ; the same multiply; a code is three bytes
        txa                     ;   before its name in the slot
        sec
        sbc     #MVNAME-MVCODE
        tax
        ldy     #0
GMVC1:  lda     FNRPLY,x
        beq     GMVC2
        sta     CDMOVE,y
        inx
        iny
        cpy     #2
        bne     GMVC1
GMVC2:  lda     #0
        sta     CDMOVE,y
        rts

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
        INCLUDE "sound.inc"
        INCLUDE "state.inc"
        INCLUDE "render.inc"
        INCLUDE "cdisp.inc"

        END
