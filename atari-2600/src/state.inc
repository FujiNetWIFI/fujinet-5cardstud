; state.inc -- the reply window, read in place.
;
; Nothing here is parsed into RAM. The server's Game struct stays where the
; cartridge painted it and the screens index it directly, which is legal
; because the mailbox repaints that window only on a SEQ commit or a slice
; select -- the reply-stability invariant in fuji_mailbox.h. On a console with
; 128 bytes of RAM, of which the stack owns the top, this is not an
; optimisation; it is the only way anything fits.
;
; Only two strings are ever copied out, and both go into the CARTRIDGE's path
; buffers rather than console RAM: the table id, which has to survive every
; /state poll after it, and the player name.

; ---------------------------------------------------------------------------
        IF      CDHASNET
; VALID8 -- sanity-check a reply before any offset in it is believed.
;
; The length test is the important one. Every offset the renderer uses is
; fixed, the window is never cleared, and a reply SHORTER than the header
; implies would have the renderer reading last poll's bytes as this poll's --
; a table that looks entirely plausible and is a hand old. playerCount sits at
; offset 153, BEFORE anything a truncation could have eaten, so it can be
; trusted to describe what should follow it.
;
; CDNUM0/1 is the sixteen-bit accumulator here. It aliases the settle loop's
; CDPRV0/1, which is dead by now: VALID8 runs after NREAD, and NREAD only runs
; once the settle loop has finished with them.
VALID8: lda     CDREQ
        cmp     #RQTABLE
        bne     VALGAME

; /tables: the count is the FIRST BYTE, not the length divided by the stride.
; Dividing looks equivalent and is not -- 181 bytes is 1 + 5*36, and the
; division says 5 only by accident of rounding.
        lda     CDRXL0
        ora     CDRXL1
        beq     VALBAD
        lda     FNRPLY+0
        cmp     #MAXTBL+1
        bcs     VALBAD
        sta     CDCNT
        lda     #FNEOK
        rts

; A Game: /state, /move/XX and /leave all answer with one.
VALGAME: lda    CDRXL1
        bne     VALG1
        lda     CDRXL0
        cmp     #GMINLEN
        bcc     VALBAD
VALG1:  lda     FNRPLY+GROUND
        cmp     #6              ; 0 waiting, 1-4 streets, 5 showdown
        bcs     VALBAD
        sta     CDROUND
        lda     FNRPLY+GNPLAY
        cmp     #MAXPLYR+1
        bcs     VALBAD
        sta     CDNPLR

; The length the header implies: 154 + 33 * playerCount, which passes 256 at
; four seats, so this is sixteen bits and not a shortcut.
        lda     #(GPLAYER)&$FF
        sta     CDNUM0
        lda     #(GPLAYER)>>8
        sta     CDNUM1
        ldx     CDNPLR
        beq     VALCMP
VALADD: lda     CDNUM0
        clc
        adc     #PLSTRID
        sta     CDNUM0
        bcc     VALA1
        inc     CDNUM1
VALA1:  dex
        bne     VALADD
VALCMP: lda     CDRXL1          ; rxlen >= want, unsigned, through the carry
        cmp     CDNUM1
        bcc     VALBAD
        bne     VALOK
        lda     CDRXL0
        cmp     CDNUM0
        bcc     VALBAD

VALOK:  lda     FNRPLY+GACTIVE
        sta     CDACT
        lda     FNRPLY+GNMOVE
        cmp     #MAXMOVE+1
        bcc     VALO1
        lda     #MAXMOVE        ; five slots are always on the wire
VALO1:  sta     CDNMV
; The spectator flag, with the shared C core's workaround: the server does not
; always send `viewing` correctly when the table is not full, so a table with
; a free seat is never a spectator's.
        lda     FNRPLY+GVIEW
        sta     CDVIEW
        lda     CDNPLR
        cmp     #MAXPLYR
        bcs     VALO2
        lda     #0
        sta     CDVIEW
VALO2:  lda     #FNEOK
        rts

VALBAD: lda     #FNEBADL
        sta     CDERR
        lda     #4
        sta     CDSTEP
        lda     #$FF
        rts

        ENDIF

        IF      CDHASUI
; ---------------------------------------------------------------------------
; CDPLYR -- FNPTRL/H points at seat X's record. Clobbers X.
;
; A record is 33 bytes and there are eight of them, so this reaches past 256
; and has to be a pointer rather than an index. Repeated addition, because
; there is no multiply: eight iterations at worst, once per seat per render.
CDPLYR: lda     #(FNRPLY+GPLAYER)&$FF
        sta     FNPTRL
        lda     #(FNRPLY+GPLAYER)>>8
        sta     FNPTRH
        cpx     #0
        beq     CDPL2
CDPL1:  lda     FNPTRL
        clc
        adc     #PLSTRID
        sta     FNPTRL
        bcc     CDPL1A
        inc     FNPTRH
CDPL1A: dex
        bne     CDPL1
CDPL2:  rts

; CDPFLD -- A = byte Y of the seat FNPTRL/H points at. An indirect LOAD, which
; is allowed; an indirect STORE is not, anywhere, because it performs a read
; at the address it writes and these pages are not driven.
CDPFLD: lda     (FNPTRL),y
        rts

; CDPW -- CDNUM0/1 = the little-endian word at offset Y of that seat.
CDPW:   lda     (FNPTRL),y
        sta     CDNUM0
        iny
        lda     (FNPTRL),y
        sta     CDNUM1
        rts

; ---------------------------------------------------------------------------
; CDCARD -- paint seat CDIDX's five cards into text rows A and A+1.
;
; Six stores, and the cartridge does the rest: five cards on a six-pixel pitch
; across planes 0-3, a rank over a pip, the bed cleared as a side effect. The
; console could not do this at all -- the pips are five pixels wide and a font
; cell is three.
;
; FH_BSL..FH_BGO are ONE-SHOT hotspots, so each is a SINGLE store with its
; data taken directly. They are not registers and must not be armed and
; committed.
CDCARD: pha                     ; the top row of the pair
        ldx     CDIDX
        jsr     CDPLYR
        lda     FNPTRL
        clc
        adc     #PLHAND
        sta     FNRSEL+FH_BSL
        lda     FNPTRH
        adc     #0              ; the carry from the line above; STA does not
                                ;   touch it
        sec
        sbc     #(FNRPLY)>>8    ; an OFFSET into the reply, not an address
        sta     FNRSEL+FH_BSH
        pla
        sta     FNRSEL+FH_BDL
        lda     #0
        sta     FNRSEL+FH_BDH
; Your own hole card is masked until the showdown. Everyone else's arrives as
; "??" already -- the server does not tell you what you are not allowed to
; know -- so this flag is only ever about seat 0, which is always you.
        lda     CDIDX
        bne     CDCA1
        lda     CDROUND
        cmp     #5
        beq     CDCA1
        lda     #FC_HIDE0
        jmp     CDCA2
CDCA1:  lda     #0
CDCA2:  sta     FNRSEL+FH_BCNT
        lda     #FB_CARD
        sta     FNRSEL+FH_BGO
        rts
        ENDIF
