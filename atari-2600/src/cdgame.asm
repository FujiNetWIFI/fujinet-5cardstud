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
GW1:    jsr     GRENDER
GRUN:   lda     #0
        sta     VBLANK
        jmp     DLOOP

; ---------------------------------------------------------------------------
; APPVBL -- the kernel's per-frame hook, called with the screen blanked.
;
; A transaction takes far longer than a frame. That is accepted here as it is
; in every sibling: the picture stalls for the round trip and comes back with
; new data, which is honest and is what the server's own pace looks like.
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
        jsr     GSEATS          ; the values, and nothing else, changed

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

APV6:   dec     CDPOLL
        beq     APV7
        rts
; Time to poll. That is a whole other bank: net.inc and url.inc are 550 bytes
; and this one has a display kernel, a renderer and a move menu to fit. The
; screen is blanked for a transaction either way, so a bank with no kernel in
; it loses nothing by taking the call.
APV7:   lda     #ENFETCH
        sta     CDENT
        lda     #BANKNET
        jmp     CDGOTO

; ---------------------------------------------------------------------------
; GFAIL -- a poll failed. Say so on the bottom bar and leave the table alone:
; the reply window still holds the last good state and it is still on screen.
GFAIL:  lda     #$FF
        sta     CDBAR
        lda     #RBAR0
        jsr     FNROWA
        ldx     #(GSNET)&$FF
        ldy     #(GSNET)>>8
        jsr     FNSETP
        jsr     FNSTRA
        lda     CDERR
        jsr     GHEX
        jsr     FNENDR
        lda     #RBAR0+1
        jsr     FNROWA
        lda     CDSTEP
        clc
        adc     #'0'
        jsr     FNCHR
        jsr     FNENDR
        jmp     GBCLR2

; GHEX -- append A as two hex digits to the row being composed.
GHEX:   pha
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jsr     GHEX1
        pla
        and     #$0F
GHEX1:  cmp     #10
        bcc     GHEX2
        adc     #6              ; carry is set here, so this adds 7 in total
GHEX2:  adc     #'0'
        jmp     FNCHR

; ---------------------------------------------------------------------------
; GRENDER -- the whole table.
;
; THE TABLE IS DRAWN BEFORE THE BANNER IS CONSIDERED. The poll that carries
; the showdown message is the same one carrying the revealed hands -- folded
; and hidden cards flip to real ones for that response only -- and by the next
; poll the server has dealt again. Every sibling port calls this out, and the
; Intellivision port had the reveal arrive four seconds after it was gone.
GRENDER: lda    CDREDRW
        beq     GR1
        lda     #0
        sta     CDREDRW
        jsr     FNCLS
GR1:    jsr     GHEAD
        jsr     GSEATS
        jsr     GCUES
        jsr     GLRCHK
        jmp     GBAR

; GHEAD -- row 0: the pot and YOUR purse. Twelve columns is exactly 'P' plus
; five digits plus '$' plus five, and the two sigils separate the numbers
; better than a space would.
GHEAD:  lda     #RHEAD
        jsr     FNROWA
        lda     #'P'
        jsr     FNCHR
        ldx     #GPOTLO
        jsr     CDSETN
        lda     #5
        jsr     CDDEC
        lda     #'$'
        jsr     FNCHR
; players[0] is always you -- the server rotates you to index 0 -- so your
; purse needs no search.
        ldx     #0
        jsr     CDPLYR
        ldy     #PLPRSLO
        jsr     CDPW
        lda     #5
        jsr     CDDEC
        jmp     FNENDR

; ---------------------------------------------------------------------------
; GSEATS -- eight seats, two rows each.
;
; The text rows are composed FIRST and the cards blitted over them, in that
; order and not the other: composing a row writes all six planes, so it clears
; the card bed as a side effect. Doing it afterwards would wipe the hand.
GSEATS: lda     #0
        sta     CDIDX
GS1:    lda     CDIDX
        cmp     CDNPLR
        bcs     GSEMPTY

        ldx     CDIDX
        jsr     CDPLYR

        ; the upper row: the card ranks, then four characters of name
        jsr     GSROW
        ldx     #4
        jsr     GNAME
        jsr     FNENDR

        ; the lower row: the pips, then the bet -- or the purse, held
        lda     CDIDX
        asl     a
        clc
        adc     #RSEAT0+1
        jsr     FNROWA
        lda     #CDCOLTX
        jsr     FNSPC
        ldx     CDIDX
        jsr     CDPLYR
        lda     CDSELHD
        beq     GSBET
        ldy     #PLPRSLO
        jmp     GSVAL
GSBET:  ldy     #PLBETLO
GSVAL:  jsr     CDPW
        jsr     CDCL4
        lda     #4
        jsr     CDDEC
        jsr     FNENDR

        ; the cards, over the bed the two rows just cleared
        lda     CDIDX
        asl     a
        clc
        adc     #RSEAT0
        jsr     CDCARD

        ; and the seat's colour, which is where its status is carried -- there
        ; is no column to spare for an F or an L
        ldx     CDIDX
        jsr     CDPLYR
        ldy     #PLSTAT
        jsr     CDPFLD
        cmp     #PSFOLD
        bcs     GSDIM           ; folded, or left
        lda     CDACT
        cmp     CDIDX
        beq     GSACT
        lda     CDIDX
        beq     GSYOU
        lda     #CGREY
        jmp     GSCOL
GSYOU:  lda     #CWHITE
        jmp     GSCOL
GSACT:  lda     #CGOLD
        jmp     GSCOL
GSDIM:  lda     #CDIM
GSCOL:  ldx     CDIDX
        sta     CDSCOL,x
        jmp     GSNEXT

; An empty seat: two blank rows, which also blanks its card bed.
GSEMPTY: jsr    GSROW
        lda     #4
        jsr     FNSPC
        jsr     FNENDR
        lda     CDIDX
        asl     a
        clc
        adc     #RSEAT0+1
        jsr     FNROWA
        lda     #FNTCOL
        jsr     FNSPC
        jsr     FNENDR
        ldx     CDIDX
        lda     #CDIM
        sta     CDSCOL,x

GSNEXT: inc     CDIDX
        lda     CDIDX
        cmp     #MAXPLYR
        beq     GSDONE          ; the seat body is far longer than a branch
        jmp     GS1             ;   can reach
GSDONE: rts

; GSROW -- begin seat CDIDX's UPPER row and skip the card bed.
GSROW:  lda     CDIDX
        asl     a               ; two rows a seat
        clc
        adc     #RSEAT0
        jsr     FNROWA
        lda     #CDCOLTX
        jmp     FNSPC

; GNAME -- append X characters of the seat's name, space-padded to X.
;
; The server sends name[9] space-padded. Stopping at the pad as well as at the
; NUL is what keeps a short name from dragging the padding into the display,
; and the one routine serves both the four-column seat field and the
; twelve-column waiting-on line.
GNAME:  ldy     #PLNAME
GNAM1:  lda     (FNPTRL),y
        beq     GNAM2
        cmp     #' '
        beq     GNAM2
        sta     FNRSEL+FH_TCHR
        iny
        dex
        bne     GNAM1
        rts
GNAM2:  lda     #' '
        sta     FNRSEL+FH_TCHR
        dex
        bne     GNAM2
        rts

; ---------------------------------------------------------------------------
; GMYTRN -- Z set if it is your turn to act.
;
; activePlayer 0 is you, $FF is nobody. A spectator never acts, and neither
; does anyone the server has offered no moves.
GMYTRN: lda     CDVIEW
        bne     GMT1
        lda     CDACT
        bne     GMT1
        lda     CDNMV
        beq     GMT1
        lda     #0
        rts
GMT1:   lda     #1
        rts

; ---------------------------------------------------------------------------
; GBAR -- the bottom four rows.
GBAR:   lda     #$FF
        sta     CDBAR
        lda     CDLRPG
        beq     GBAR1
        jmp     GBLR            ; a banner page is up
GBAR1:  jsr     GMYTRN
        beq     GBMENU
        jmp     GBWAIT

; The move menu, one move a row so the red bar can be the full width of one.
; Four rows and up to five moves, so the window scrolls to keep the cursor in
; it -- which is rare: FOLD/CHECK/BET and FOLD/CALL/RAISE are the common
; shapes.
GBMENU: lda     CDSEL
        cmp     CDNMV
        bcc     GBM1
        lda     #0
        sta     CDSEL
GBM1:   lda     CDSEL           ; keep the cursor inside the window
        cmp     CDMVTOP
        bcs     GBM2
        sta     CDMVTOP
GBM2:   lda     CDSEL
        sec
        sbc     #NBARROW-1
        bcc     GBM3
        cmp     CDMVTOP
        bcc     GBM3
        sta     CDMVTOP
GBM3:   lda     #0
        sta     CDIDX
GBM4:   lda     CDIDX
        clc
        adc     #RBAR0
        jsr     FNROWA
        lda     CDIDX
        clc
        adc     CDMVTOP
        cmp     CDNMV
        bcs     GBMBLK          ; past the last move: a blank row
        sta     CDTMP
        ; the red bar goes on the selected move's row
        cmp     CDSEL
        bne     GBM5
        lda     CDIDX
        clc
        adc     #RBAR0
        sta     CDBAR
; validMoves[i].name is ten bytes on the wire and is padded here to ten
; columns, so the move clock always lands in the last two whatever the name.
GBM5:   lda     CDIDX
        clc
        adc     CDMVTOP
        jsr     GMVOFF
        ldy     #10
        jsr     FNRPLA
        ; the move clock, in the last two columns of the selected row
        lda     CDIDX
        clc
        adc     #RBAR0
        cmp     CDBAR
        bne     GBM7
        lda     CDCLK
        sta     CDNUM0
        lda     #0
        sta     CDNUM1
        lda     #2
        jsr     CDDEC
        jmp     GBM8
GBM7:   lda     #2
        jsr     FNSPC
GBM8:   jsr     FNENDR
        jmp     GBMNXT
GBMBLK: lda     #FNTCOL
        jsr     FNSPC
        jsr     FNENDR
GBMNXT: inc     CDIDX
        lda     CDIDX
        cmp     #NBARROW
        bne     GBM4
        rts

; GMVOFF -- X = the reply offset of move A's name. Every move slot is inside
; the first 256 bytes of the reply, so one byte of index reaches all five.
GMVOFF: sta     CDTMP
        asl     a               ; i*2
        asl     a               ; i*4
        asl     a               ; i*8
        clc
        adc     CDTMP           ; i*9
        adc     CDTMP           ; i*10
        adc     CDTMP           ; i*11
        adc     CDTMP           ; i*12
        adc     CDTMP           ; i*13 = i*MVSTRID
        clc
        adc     #GMOVES+MVNAME
        tax
        rts

; Not your turn: who the table is waiting on, and where the hand is.
GBWAIT: lda     #RBAR0
        jsr     FNROWA
        lda     CDVIEW
        beq     GBW1
        ldx     #(GSVIEW)&$FF
        ldy     #(GSVIEW)>>8
        jmp     GBW3
GBW1:   lda     CDACT
        bmi     GBW2            ; $FF: nobody is to act
        ldx     #(GSWAIT)&$FF
        ldy     #(GSWAIT)>>8
        jmp     GBW3
GBW2:   ldx     #(GSRND)&$FF
        ldy     #(GSRND)>>8
GBW3:   jsr     FNSETP
        jsr     FNSTRA
        jsr     FNENDR
; The second row names the seat, or the round.
        lda     #RBAR0+1
        jsr     FNROWA
        lda     CDACT
        bmi     GBW4
        lda     CDACT
        cmp     #MAXPLYR
        bcs     GBW4
        sta     CDIDX
        ldx     CDIDX
        jsr     CDPLYR
        ldx     #FNTCOL         ; the whole row: a nine-character name fits
        jsr     GNAME
        jsr     FNENDR
        jmp     GBW5
GBW4:   lda     CDROUND
        clc
        adc     #'0'
        jsr     FNCHR
        jsr     FNENDR
GBW5:   jmp     GBCLR2

; GBCLR2 -- blank the last two bar rows. Both the waiting line and the
; failure line are two rows long, and the four-row move menu that was there
; before them is not.
GBCLR2: lda     #RBAR0+2
        jsr     FNROWA
        jsr     FNENDR
        lda     #RBAR0+3
        jsr     FNROWA
        jmp     FNENDR

; ---------------------------------------------------------------------------
; The end-of-hand banner.
;
; Fired on a CHECKSUM of lastResult, not on the round. Triggering off the
; round is wrong twice over: at a small table where everyone folds, the server
; finishes the hand and deals the next one without the round ever leaving 1,
; so there is no edge to see -- and even at a full table the showdown can fall
; between two polls. What always changes is lastResult itself. A checksum can
; collide, in which case one banner is missed; comparing eighty bytes every
; poll to avoid that is not worth the bank space.
GLRCHK: ldx     #0
        lda     #0
GLR1:   clc
        adc     FNRPLY+GLAST,x
        inx
        cpx     #20
        bne     GLR1
        cmp     CDLRSUM
        beq     GLR3            ; nothing new
        sta     CDLRSUM
; An empty lastResult is not a result. The first poll after sitting down arms
; the baseline without announcing it, which is what CDLRSUM starting at zero
; does for free: a NUL-padded field sums to zero and matches.
        lda     FNRPLY+GLAST
        beq     GLR3
        lda     #1
        sta     CDLRPG
        lda     #LRHOLD
        sta     CDLRHLD
        lda     #SNDEND
        jsr     SNDFIRE
GLR3:   rts

; GLRNEXT -- the hold expired: the next page, or done.
;
; It stops at the first page with nothing on it rather than always showing
; two, so a short message does not sit behind a blank one.
GLRNEXT: lda    CDLRPG
        clc
        adc     #1
        sta     CDTMP
        sec
        sbc     #1
        asl     a
        asl     a               ; four rows a page
        jsr     GLRROW
        beq     GLRN2           ; nothing there: the message is finished
        lda     CDTMP
        sta     CDLRPG
        lda     #LRHOLD
        sta     CDLRHLD
        jmp     GBAR
GLRN2:  lda     #0
        sta     CDLRPG
        jmp     GBAR

; ---------------------------------------------------------------------------
; The banner's line breaking.
;
; lastResult is up to eighty-one bytes -- "Fry BOT won with Pair, Sixes" -- and
; the bar is four rows of twelve, so it pages. Breaking every twelve
; characters regardless splits words down the middle, which at this size is
; the difference between a sentence and a jumble.
;
; There is no room to remember where a row started: every byte of $80-$BF is
; spoken for. So a row's start is RECOMPUTED by walking the message from the
; beginning and counting rows -- eight walks of twelve bytes, once every
; second and a half, against one byte of RAM this client does not have. The
; page number alone is enough state, which is also what makes the page timer
; and the hold work without any of this being stored.

; GLRSET -- FNPTRL/H -> lastResult + A.
GLRSET: clc
        adc     #(FNRPLY+GLAST)&$FF
        sta     FNPTRL
        lda     #0
        adc     #(FNRPLY+GLAST)>>8
        sta     FNPTRH
        rts

; GLRLEN -- A = how many of the twelve characters at FNPTRL/H belong on one
; row. Twelve, unless that would cut a word, in which case the last space in
; the row. A word longer than the row still breaks hard -- there is nowhere
; else for it to go.
GLRLEN: ldy     #FNTCOL
        lda     (FNPTRL),y      ; the character just past a full row
        beq     GLRL12          ; the message ends inside this row
        cmp     #' '
        beq     GLRL12          ; it breaks cleanly anyway
        ldy     #FNTCOL-1
GLRL1:  lda     (FNPTRL),y
        cmp     #' '
        beq     GLRL2
        dey
        bne     GLRL1
GLRL12: lda     #FNTCOL
        rts
GLRL2:  tya                     ; the space itself is dropped, not printed
        rts

; GLRADV -- FNPTRL/H += A, then past a space if the break landed on one.
GLRADV: clc
        adc     FNPTRL
        sta     FNPTRL
        bcc     GLRA1
        inc     FNPTRH
GLRA1:  ldy     #0
        lda     (FNPTRL),y
        cmp     #' '
        bne     GLRA2
        inc     FNPTRL
        bne     GLRA2
        inc     FNPTRH
GLRA2:  rts

; GLRROW -- FNPTRL/H -> the start of message row A. Z set if that row is past
; the end of the message.
GLRROW: sta     CDTMP
        lda     #0
        jsr     GLRSET
GLRR1:  lda     CDTMP
        beq     GLRR2
        ldy     #0
        lda     (FNPTRL),y
        beq     GLRR3           ; the message ran out before this row
        jsr     GLRLEN
        jsr     GLRADV
        dec     CDTMP
        jmp     GLRR1
GLRR2:  ldy     #0
        lda     (FNPTRL),y      ; Z set if the row is empty
        rts
GLRR3:  lda     #0
        rts

; GBLR -- four rows of the current page.
GBLR:   lda     #0
        sta     CDIDX
GBLR1:  lda     CDIDX
        clc
        adc     #RBAR0
        jsr     FNROWA
        lda     CDLRPG
        sec
        sbc     #1
        asl     a
        asl     a
        clc
        adc     CDIDX
        jsr     GLRROW
        beq     GBLRB
        jsr     GLRLEN
        sta     CDTMP           ; GLRROW is finished with it
        ldy     #0
GBLRE:  cpy     CDTMP
        bcs     GBLRP
        lda     (FNPTRL),y
; Stop at the NUL as well as at the count. GLRLEN reports a full twelve when
; the message ENDS inside the row -- there is no word to protect, so there is
; nothing to back up to -- and the bytes past the terminator are NUL, which
; the cartridge's font renders as '?'. The last page of every banner read
; "EIGHTS??????".
        beq     GBLRP
        sta     FNRSEL+FH_TCHR
        iny
        jmp     GBLRE
GBLRP:  cpy     #FNTCOL         ; pad, so a shorter row leaves no debris
        bcs     GBLRD
        lda     #' '
        sta     FNRSEL+FH_TCHR
        iny
        jmp     GBLRP
GBLRB:  lda     #FNTCOL
        jsr     FNSPC
GBLRD:  jsr     FNENDR
        inc     CDIDX
        lda     CDIDX
        cmp     #NBARROW
        bne     GBLR1
        rts

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
; GCUES -- the sound cues, on edges rather than on states.
GCUES:  lda     CDROUND
        cmp     CDPRVRD
        beq     GC1
        sta     CDPRVRD
        lda     #SNDDEAL
        jsr     SNDFIRE
GC1:    rts

GSNET:  DB      "NET ERR ",0
GSWAIT: DB      "WAITING ON",0
GSRND:  DB      "ROUND",0
GSVIEW: DB      "VIEWING",0

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
        INCLUDE "cdisp.inc"

        END
