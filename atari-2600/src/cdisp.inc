; cdisp.inc -- the 48-pixel text display, with colour.
;
; The cartridge composes the glyphs (firmware/src/vcs_render.c) and publishes
; them as six 128-byte planes at $1800-$1AFF; this is the console half.
;
; THE 53-CYCLE INK BODY BELOW IS TRANSCRIBED FROM
; fujinet-firmware/pico/atari-2600/testrom/fujidisp.inc AND MUST NOT BE
; RETIMED. Its four constants are the coordinates of a ONE-PIXEL-WIDE window
; found by byte-comparing the raster against the renderer with
; emu/dispcheck.py -- every wrong setting still looks like text.
;
; WHAT IS ADDED HERE: colour, for nothing.
;
; vcs_render_row() writes ZERO to scanline 5 of every cell -- the sixth line
; of a 4x6 cell is blank leading and the renderer does not store it. So the
; loop is 21 rows of (5 cycle-exact ink lines + 1 SEAM LINE), the seam line is
; a line on which no glyph can possibly be drawn, and it is free for whatever
; the kernel wants to do instead. That costs no scanlines at all: the blank
; line was already there, and the picture is still 21*6 = 126 lines.
;
; "Free" means free of DEADLINES, not free of cycles: the seam line still has
; to finish inside its 76, or it quietly costs a second scanline. The budget is
; spelled out where it is spent, at the seam line itself.
;
; Three facts make the seam line's timing a non-problem, which is the whole
; trick:
;
;   * Its sprite writes are BLANKING writes. The copies draw nothing whatever
;     cycle the stores land on.
;   * COLUBK has until the green playfield border ends at pixel 31, about
;     cycle 32, because the playfield draws OVER the background -- a
;     background change underneath the border cannot be seen. That is a
;     33-cycle window for a compare and a branch, not the 22 of hblank.
;   * COLUP0/COLUP1 have NO deadline on this line. The sprites are blank, so a
;     new ink colour cannot show until the next line anyway.
;
; What that buys: a green frame, a black panel, a full-width red bar under the
; selected move, and a per-seat ink colour that says who folded and who is to
; act -- with the ink body left byte-identical on every line that draws.

; ---------------- kernel timing constants ----------------
; Not taste. See the file header.
RNOP    EQU     16              ; NOPs before RESP0 (2 cycles each)...
R3      EQU     1               ; ...plus this, so RESP0 lands on cycle 38
HM0     EQU     15              ; HMP0 = $F0: move P0 right one pixel...
HM1     EQU     0               ; ...and leave P1, closing the gap to eight
VDEL    EQU     1               ; vertical delay on both players

; ---------------- text plane bases ----------------
TP0     EQU     FNTEXT+$000
TP1     EQU     FNTEXT+$080
TP2     EQU     FNTEXT+$100
TP3     EQU     FNTEXT+$180
TP4     EQU     FNTEXT+$200
TP5     EQU     FNTEXT+$280

TINK    EQU     FNTCLH-1        ; 5 ink lines, then the seam
TLINES  EQU     FNTROW*FNTCLH   ; 126 scanlines of text

; The seam line turns the row into a seat index with DEX rather than a
; four-cycle SEC/SBC, which is only the same arithmetic while the first seat
; row is row 1. It is, and the picture is 126 scanlines only because that DEX
; fits -- so this is an assembly-time failure rather than a comment.
        IF      RSEAT0<>1
        ERROR   "the seam line's DEX assumes RSEAT0 = 1"
        ENDIF

; NTSC: 3 vsync + 37 vblank + 192 visible + 30 overscan = 262.
VBLINES EQU     36              ; plus the one DLOOP spends landing the
                                ;   unblank on a line boundary
OSLINES EQU     30
PADLINE EQU     (192-TLINES)/2  ; 33 above and below

; The green side border. The playfield is 40 pixels of four clocks; PF0 is
; pixels 0-3 in its HIGH nibble and PF1 pixels 4-11 drawn MSB first, so $F0 in
; each lights pixels 0-7 -- clocks 0-31. With CTRLPF's reflect bit the right
; half mirrors, putting the same 32 clocks at 128-159. The 48-pixel text block
; sits at pixels 46-94 and never comes near either.
PFEDGE  EQU     $F0

; ---------------- DINIT: TIA setup and player positioning ----------------
DINIT:
; Two players, three copies each, eight pixels apart: P0 draws groups 0,2,4
; and P1 groups 1,3,5, interleaving to 48 contiguous pixels. VDELP is what
; makes the six-write sequence land in the right copies.
        lda     #$03            ; NUSIZ: three copies, close
        sta     NUSIZ0
        sta     NUSIZ1
        lda     #VDEL
        sta     VDELP0
        sta     VDELP1
        lda     #CWHITE
        sta     COLUP0
        sta     COLUP1
        lda     #CGREEN
        sta     COLUBK
        sta     COLUPF
        lda     #$01            ; CTRLPF: reflect, so one nibble draws both
        sta     CTRLPF          ;   edges
        lda     #PFEDGE
        sta     PF0
        sta     PF1
        lda     #0
        sta     PF2
        sta     CDBAR           ; no red bar until a menu asks for one
        sta     CDFRAME

; Player positioning, transcribed from batari Basic's std_kernel.asm rather
; than re-derived. RESP0 strobed at cycle 43, RESP1 three cycles later (nine
; pixels, since two stores cannot be closer), then HMP0/HMP1 pull them to
; eight apart. RDLY is built from NOPs plus an optional 3-cycle zero-page
; store, because the copies are 2.67 cycles apart and NOPs alone only reach
; even delays.
        sta     WSYNC
        REPT    RNOP
        nop
        ENDM
        IF      R3
        sta     PAD3            ; 3 cycles, so odd delays are reachable
        ENDIF
        sta     RESP0
        sta     RESP1
        lda     #HM0*16
        sta     HMP0
        lda     #(HM1*16)&$FF
        sta     HMP1
; An EARLY HMOVE -- WSYNC, then strobe -- rather than batari's late one at
; cycle 73. A late HMOVE does not apply the motion nibbles cleanly: nudging
; HMP0 by one there widened the player gap from 8 pixels to 9 instead of
; sliding the block, and the window this kernel needs is one pixel wide.
        sta     WSYNC
        sta     HMOVE
        rts

; ---------------- DLOOP: draw frames, forever ----------------
DLOOP:
        lda     #2
        sta     VSYNC
        sta     WSYNC
        sta     WSYNC
        sta     WSYNC
        lda     #0
        sta     VSYNC

        lda     #2
        sta     VBLANK
        ldx     #VBLINES
DVBLP:  sta     WSYNC
        dex
        bne     DVBLP
; The client's per-frame hook, called while the screen is still blanked and
; the raster is nowhere near the text. Input scanning belongs here; so does a
; transaction, with the caveat that a network round trip takes far longer than
; a frame and the picture will stall for it.
        inc     CDFRAME
        jsr     APPVBL

; Land the unblank on a LINE BOUNDARY. APPVBL returns at whatever cycle it
; happens to finish on, and clearing VBLANK there unblanks the raster
; mid-scanline: the first visible line came up black for its first forty
; pixels and green for the rest, which on a felt-coloured band is a visible
; notch in the top left corner.
        sta     WSYNC
        lda     #0
        sta     VBLANK

; The top band is felt, and its LAST line hands over to row 0: the store lands
; a couple of cycles into hblank, so that line is already the panel's colour
; and no scanline is spent on the transition.
        ldx     #PADLINE-1
DPADA:  sta     WSYNC
        dex
        bne     DPADA
        sta     WSYNC
        lda     CDBGT           ; row 0's background
        sta     COLUBK
        lda     #CWHITE
        sta     COLUP0
        sta     COLUP1

; ---------------- the kernel ----------------
; The stack pointer is a spare register in the ink body, and that is not a
; flourish: the six player copies are 2.67 cycles apart while a zero-page
; store is 3, so the four late writes drift a full cycle across the block.
; Recovering a byte from zero page costs 3 cycles; `tsx` costs 2, and that one
; cycle is the difference between a schedule that closes and one that does
; not. It is also why the seventh write is `sty GRP0`: its value is a
; don't-care, so Y serves and no register need be spared.
;
; NOTHING between the txs here and the restore below may touch the stack --
; no JSR -- which is why the row loop and the seam line are inline. The 6507
; has no interrupts, so there is nothing else to worry about.
        tsx
        stx     SAVSP
        ldy     #0              ; the absolute scanline, 0-125; never clobbered
        lda     #0
        sta     CDROW

DROWL:  lda     #TINK
        sta     CDLINE

DINK:   sta     WSYNC
        lda     TP0,y           ; 4
        sta     GRP0            ; 3   ->  7   new GRP0 = group 0
        lda     TP1,y           ; 4   -> 11
        sta     GRP1            ; 3   -> 14   displays group 0
        lda     TP2,y           ; 4   -> 18
        sta     GRP0            ; 3   -> 21   displays group 1
        lda     TP4,y           ; 4   -> 25
        tax                     ; 2   -> 27
        txs                     ; 2   -> 29   park group 4 in S
        lda     TP3,y           ; 4   -> 33
        tax                     ; 2   -> 35
        lda     TP5,y           ; 4   -> 39
        stx     GRP1            ; 3   -> 42   displays group 2
        tsx                     ; 2   -> 44   recover group 4
        stx     GRP0            ; 3   -> 47   displays group 3
        sta     GRP1            ; 3   -> 50   displays group 4
        sty     GRP0            ; 3   -> 53   displays group 5; value unused
        iny                     ; 2   -> 55
        dec     CDLINE          ; 5   -> 60
        bne     DINK            ; 3   -> 63

; ---- the seam line ----
; Line 5 of the cell. The cartridge stores zero here, so no glyph can be
; drawn and the kernel does something useful instead. See the file header for
; why none of this needs to be cycle-exact WITHIN the line.
;
; IT DOES, HOWEVER, HAVE TO FIT IN ONE. This is a WSYNC-bounded loop, so a
; seam line that runs past cycle 76 does not glitch -- it silently costs a
; SECOND scanline, because the next `sta WSYNC` is then already on the
; following line and waits for the end of THAT one. The first cut of this
; reached DROWL's `lda #TINK` at cycle 77 on the seat path and every seat row
; was seven scanlines instead of six: 142 lines of text, a 278-line frame, a
; picture that rolls on anything that cares. Nothing about it looked wrong on
; screen, which is the whole problem with the failure mode.
;
; The budget is therefore from this WSYNC to the last cycle of the next one,
; and it is 76. The seat path below reaches it at 72. Two of the four spare
; cycles are already spoken for: `lda CDBGT,x` costs a fifth cycle in any bank
; whose twenty-two byte table straddles a page (cdgame's does), and
; `bne DSEAM1` a third if its target ever lands on another page. So there are
; TWO cycles free here. Anything bigger has to go on the chrome path, which
; reaches its WSYNC at 66 and has ten.
        sta     WSYNC
        lda     #0              ; 2
        sta     GRP0            ; 3   ->  5   three writes, not two: with
        sta     GRP1            ; 3   ->  8     VDELP set a write to GRP0
        sta     GRP0            ; 3   -> 11     reloads the DELAYED GRP1
        ldx     CDROW           ; 3   -> 14
        inx                     ; 2   -> 16   the row these colours are FOR
        lda     CDBGT,x         ; 4   -> 20
        cpx     CDBAR           ; 3   -> 23
        bne     DSEAM1          ; 3   -> 26
        lda     #CRED           ; 2         the selected move's bar
DSEAM1: sta     COLUBK          ; 3   -> 29   by cycle 30, pixel 22 at the
                                ;   latest, under the green border
        stx     CDROW           ; 3   -> 32   the row counter, advanced here
                                ;   rather than with a read-modify-write at
                                ;   the bottom: X still holds it, and `inc`
                                ;   costs five cycles this line has not got
        iny                     ; 2   -> 34   step past the blank plane byte.
                                ;   AFTER the COLUBK store, not before: it has
                                ;   to land before the green border ends at
                                ;   pixel 31 and two cycles is two-thirds of
                                ;   the margin that had.
; The ink colour: chrome for the bottom bar, otherwise this seat's, decided
; once per render and parked in CDSCOL because the kernel cannot afford to
; walk the reply window.
;
; There is no low bound test. This seam programmes row X = CDROW + 1, which is
; 1-21, so `cpx #RSEAT0 / bcc` could never branch -- four cycles spent on a
; comparison whose answer was already known. Row 0 is chrome because DLOOP
; writes CWHITE before the kernel starts, not because of a test here.
        cpx     #RBAR0          ; 2   -> 36
        bcs     DSEAM2          ; 2   -> 38   out of line; see below
        dex                     ; 2   -> 40
        txa                     ; 2   -> 42
        lsr     a               ; 2   -> 44   two rows per seat
        tax                     ; 2   -> 46
        lda     CDSCOL,x        ; 4   -> 50
DSEAM3: sta     COLUP0          ; 3   -> 53
        sta     COLUP1          ; 3   -> 56
        lda     CDROW           ; 3   -> 59
        cmp     #FNTROW         ; 2   -> 61
        bne     DROWL           ; 3   -> 64   + DROWL's 5 and the WSYNC's 3

        ldx     SAVSP
        txs

; The last seam line already blanked the sprites and set the background back
; to felt -- CDBGT's twenty-second entry exists for exactly that -- so the
; bottom band needs nothing but scanlines.
        ldx     #PADLINE
DPADB:  sta     WSYNC
        dex
        bne     DPADB

        lda     #2
        sta     VBLANK
        ldx     #OSLINES
DOSLP:  sta     WSYNC
        dex
        bne     DOSLP
        jmp     DLOOP

; The chrome rows' ink colour, OUT OF LINE. The seat path is the one with no
; cycles left, so it gets the fall-through and the bottom bar pays the three
; for a JMP -- it reaches the next WSYNC at 66 either way.
DSEAM2: lda     #CWHITE
        jmp     DSEAM3

; ---------------------------------------------------------------------------
; CDBGT is NOT here. Each bank supplies its own twenty-two byte table, because
; each bank is a different screen: the table has eight two-row seats that need
; alternating bands to separate them, and the lobby has a list that does not.
;
; Twenty-TWO entries, not twenty-one: the seam line at the bottom of row 20
; programmes row 21, which does not exist, and making that entry felt is what
; turns the handover to the bottom band into a colour the kernel was going to
; write anyway.
