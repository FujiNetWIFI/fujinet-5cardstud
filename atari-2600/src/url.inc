; url.inc -- the devicespec, streamed straight into the TX page a character at
; a time.
;
; Nothing is assembled in RAM. The cartridge counts the payload itself, so
; nothing here tracks a length -- which is the only reason a console with 128
; bytes can build
;
;   N:https://5card.carr-designs.com/move/CA?table=ai1&player=VCS&bin=1
;
; at all. The table id and the player name are not in console RAM either: they
; live in the cartridge's path buffers and FP_TXRAW streams them from there
; into the request without their ever crossing the bus.
;
; A fixed-width wire field is pushed up to its NUL and NEVER through it. The
; server sends name[9] space-padded, and padding in the middle of a URL
; corrupts the request -- the Arcadia port calls this out by name.
;
; All four requests are built here, in the one bank that issues any of them.
; An earlier arrangement guarded them per bank, which was pointless once the
; fetch got a bank to itself: the strings are 60 bytes and they now exist
; once.

; UTXS -- append the NUL-terminated string at FNPTRL/FNPTRH to the TX stream.
UTXS:   ldy     #0
UTXS1:  lda     (FNPTRL),y
        beq     UTXS2
        sta     FNTX
        iny
        bne     UTXS1
UTXS2:  rts

; UPUT -- point at the string in X (lo) / Y (hi) and append it.
UPUT:   jsr     FNSETP
        jmp     UTXS

; ---------------------------------------------------------------------------
; BLDURL -- stream the devicespec for the request in CDREQ.
BLDURL: lda     #'N'
        sta     FNTX
        lda     #':'
        sta     FNTX
        ldx     #(UENDPT)&$FF
        ldy     #(UENDPT)>>8
        jsr     UPUT

        lda     CDREQ
        cmp     #RQTABLE
        bne     BU1
; /tables carries its own query and takes neither a table nor a player.
        ldx     #(UTABLS)&$FF
        ldy     #(UTABLS)>>8
        jmp     UPUT
BU1:    cmp     #RQLEAVE
        bne     BU2
        ldx     #(ULEAVE)&$FF
        ldy     #(ULEAVE)>>8
        jsr     UPUT
        jmp     BUQ
BU2:    cmp     #RQMOVE
        beq     BUMOVE
        ldx     #(USTATE)&$FF
        ldy     #(USTATE)>>8
        jsr     UPUT
        jmp     BUQ
; /move/XX REPLACES /state for this poll -- its reply IS the next state, so
; there is no separate submit anywhere in this client.
BUMOVE: ldx     #(UMOVE)&$FF
        ldy     #(UMOVE)>>8
        jsr     UPUT
        ldx     #0
BUM1:   lda     CDMOVE,x
        beq     BUQ
        sta     FNTX
        inx
        cpx     #2             ; a move code is two characters and a NUL
        bne     BUM1

; The query. Both halves come out of the cartridge.
BUQ:    ldx     #(UQTAB)&$FF
        ldy     #(UQTAB)>>8
        jsr     UPUT
        lda     #PBTABLE
        jsr     FNWSEL
        jsr     FNWRAW
        ldx     #(UQPLR)&$FF
        ldy     #(UQPLR)>>8
        jsr     UPUT
        lda     #PBNAME
        jsr     FNWSEL
        jsr     FNWRAW
        ldx     #(UQBIN)&$FF
        ldy     #(UQBIN)>>8
        jmp     UPUT

        INCLUDE "../build/endpoint.inc"

UTABLS: DB      "tables?bin=1",0
USTATE: DB      "state",0
UMOVE:  DB      "move/",0
ULEAVE: DB      "leave",0
UQTAB:  DB      "?table=",0
UQPLR:  DB      "&player=",0
; No &be=1. The Go server's binary.BigEndian path is the road much less
; travelled, and the Intellivision port found pot and bet corruption on it
; that always landed on exact multiples of 256.
UQBIN:  DB      "&bin=1",0
