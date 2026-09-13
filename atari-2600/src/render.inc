; render.inc -- everything that COMPOSES the table.
;
; Pulled out of cdgame.asm so two banks can carry it. cdgame keeps it for the
; small repaints it does from APPVBL -- the move bar on a cursor key, the seat
; values on a SELECT -- and cdcomp carries a second copy for the whole-table
; recompose after a poll, which is far too big to do inside a frame.
;
; WHY THAT SPLIT EXISTS. Composing twenty-one rows is about 12,900 cycles and
; the display kernel's vblank budget is 2,812, so the recompose lengthened the
; frame it ran in from 262 scanlines to about 430 -- and a frame of a
; different length is a picture in a different place, once every poll. cdgame
; had 32 bytes left and no room to fix it in place. cdcomp has two thousand,
; and spends them on a blanked frame of exactly the right length.
;
; Nothing here reads input or decides anything: it composes rows into the
; cartridge and returns. The state it composes from is the reply window and
; zero page, both of which survive a bank switch.
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
GS1:    jsr     GSEAT1
        inc     CDIDX
        lda     CDIDX
        cmp     #MAXPLYR
        bne     GS1
        rts

; GSEAT1 -- compose the ONE seat CDIDX, and the reason GSEATS is a loop around
; it rather than a loop with a body: cdcomp composes a seat at a time, fitting
; as many as will go into a frame's vblank, so that a recompose never costs a
; frame that does not draw. See cdcomp.asm.
GSEAT1: lda     CDIDX
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
        rts

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
        rts

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
