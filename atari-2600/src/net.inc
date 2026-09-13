; net.inc -- the N: device round trip.
;
; A 6502 transcription of arcadia/net.inc's sequence, which is the astrocade's,
; which is the Odyssey 2's. The rules it carries forward were each learned
; somewhere else in this family and none of them is optional:
;
;  * The CLOSE belongs at the START of the next request, never after the READ.
;    Every transaction repaints the whole reply window, a CLOSE reply is
;    empty, and every screen here renders straight out of that window BETWEEN
;    polls -- so closing after the read wipes the thing being displayed.
;  * FNRXLO/FNRXHI belong to the MOST RECENT transaction, so the reply length
;    is captured immediately after the READ and not a transaction later.
;  * The ESP32 reports STATUS as soon as SOME of the response has arrived.
;    Poll until two consecutive readings agree AND the count has reached a
;    caller-supplied minimum. Two agreeing readings alone is not enough: the
;    Channel F port watched a /state fetch settle at 129 bytes of a 385-byte
;    response and hand back a plausible header followed by forty bytes of
;    nothing.
;  * The 4th STATUS byte is nDevStatus_t. The HTTP GET is DEFERRED until the
;    first STATUS call, so that byte is the only place an HTTP error is
;    visible -- an error page has a perfectly readable body and a perfectly
;    plausible length.
;  * Unsigned compares go through the CARRY, never the sign bit. The Channel F
;    port's settle loop used the sign and 181 - 1 came out negative, so a
;    settled 181-byte reply read as short of a one-byte minimum and the poll
;    never terminated.
;
; Every routine returns A = 0 on success and sets Z accordingly.

; NCLOSE -- close the previous request's connection. The first call has
; nothing to close and earns a NAK; both results are ignored on purpose.
NCLOSE: lda     #NETDEV
        sta     FNDEV
        lda     #NCCLOSE
        sta     FNCMD
        lda     #0
        sta     FNNPR
        jsr     FNBEG
        jsr     NPGO
        lda     #0
        rts

; NOPEN -- OPEN the devicespec as an HTTP GET with no translation.
; The devicespec is streamed by BLDURL straight into the TX page; nothing is
; assembled in RAM, because there is no RAM to assemble it in.
NOPEN:  lda     #NETDEV
        sta     FNDEV
        lda     #NCOPEN
        sta     FNCMD
        lda     #2
        sta     FNNPR
        jsr     FNBEG
        lda     #NMHTTPG
        jsr     FNPB
        lda     #NTRNONE
        jsr     FNPB
        jsr     BLDURL
        jsr     NPGO
        cmp     #FNEOK
        bne     NOBAD
        jsr     FNACK
        cmp     #FNEOK
        bne     NOBAD
        lda     #FNEOK
        rts
NOBAD:  sta     CDERR
        lda     #1
        sta     CDSTEP
        lda     #$FF
        rts

; NSTATUS -- one STATUS poll. On success CDAVL0/1 is the byte count waiting.
NSTATUS: lda    #NETDEV
        sta     FNDEV
        lda     #NCSTAT
        sta     FNCMD
        lda     #2
        sta     FNNPR
        jsr     FNBEG
        lda     #0
        jsr     FNPB
        lda     #0
        jsr     FNPB
        jsr     NPGO
        cmp     #FNEOK
        bne     NSBAD
        jsr     FNACK
        cmp     #FNEOK
        bne     NSBAD
        lda     FNRPLY+NSDEVST
        cmp     #1              ; SUCCESS -- anything else is an HTTP error,
        bne     NSBAD2          ;   and its body would read as a short reply
        lda     FNRPLY+NSAVLO
        sta     CDAVL0
        lda     FNRPLY+NSAVHI
        sta     CDAVL1
        lda     #FNEOK
        rts
NSBAD2: lda     FNRPLY+NSDEVST  ; report what the server actually said
NSBAD:  sta     CDERR
        lda     #2
        sta     CDSTEP
        lda     #$FF
        rts

; NRCAP -- A (low) / X (high) = min(CDAVL, the read cap for this request).
;
; The cap is not caution. A Game is 418 bytes and the reply window is 512, so
; a reply that fits is a reply that can be read IN PLACE with no slice paging
; at all -- which is the whole reason the window is 512 and not 256.
NRCAP:  lda     CDREQ
        cmp     #RQTABLE
        bne     NRC1
        lda     #(TMAXLEN)&$FF
        ldx     #(TMAXLEN)>>8
        jmp     NRC2
NRC1:   lda     #(GMAXLEN)&$FF
        ldx     #(GMAXLEN)>>8
NRC2:   cpx     CDAVL1
        bcc     NRC3            ; the cap's high byte is lower: the cap wins
        bne     NRC4            ; it is higher: what is available wins
        cmp     CDAVL0
        bcc     NRC3
NRC4:   lda     CDAVL0
        ldx     CDAVL1
NRC3:   rts

; NREAD -- READ min(available, cap) bytes, then capture the reply length.
NREAD:  lda     #NETDEV
        sta     FNDEV
        lda     #NCREAD
        sta     FNCMD
        lda     #1
        sta     FNNPR
        jsr     FNBEG
        jsr     NRCAP
        jsr     FNPW            ; ONE parameter of TWO bytes, not two of one
        jsr     NPGO
        cmp     #FNEOK
        bne     NRBAD
        jsr     FNACK
        cmp     #FNEOK
        bne     NRBAD
        lda     FNRXLO          ; capture NOW: the next transaction's reply
        sta     CDRXL0          ;   length would replace it, and it is always 0
        lda     FNRXHI
        sta     CDRXL1
        lda     #FNEOK
        rts
NRBAD:  sta     CDERR
        lda     #3
        sta     CDSTEP
        lda     #$FF
        rts

; NSMIN -- CDMIN = the byte count this request will not settle below.
;
; A Game's fixed prefix runs through playerCount at offset 153, so 154 is the
; point at which every offset the renderer uses is real. A /tables listing has
; no such floor -- an empty lobby is one byte -- and neither has /leave.
NSMIN:  lda     CDREQ
        cmp     #RQSTATE
        beq     NSM1
        cmp     #RQMOVE
        beq     NSM1
        lda     #1
        ldx     #0
        jmp     NSM2
NSM1:   lda     #(GMINLEN)&$FF
        ldx     #(GMINLEN)>>8
NSM2:   sta     CDMIN0
        stx     CDMIN1
        rts

; ---------------------------------------------------------------------------
; NFRAME -- one frame of waiting, and the whole reason this bank has a display
; kernel in it.
;
; For a game poll that is a REAL frame: the text planes still hold the table
; the game bank composed out of the last reply, and the reply window is not
; repainted until the READ, so redrawing it is both correct and free. For the
; lobby's one-off RQTABLE and RQLEAVE it is the old blind delay, because this
; bank carries the table screen's CDBGT and only that one -- and those two
; happen on a screen change, where nobody can see a blank.
;
; It clobbers X and Y on both paths, which is why the settle loop counts its
; repeats in CDDLY. Battleship's first caller counted in Y as well and every
; wait ran 256 times over -- seventy seconds where seventeen milliseconds were
; meant, which looks exactly like a server that never answers.
NFRAME: lda     CDREQ
        cmp     #RQSTATE
        beq     NFDRAW
        cmp     #RQMOVE
        beq     NFDRAW
        jmp     CDWAIT
NFDRAW: jmp     DFRAME

; ---------------------------------------------------------------------------
; NPGO -- FNGO, paced by the display instead of by a spin loop.
;
; The fixed tail's FNGO waits on ACKSEQ in a three-deep counted loop with the
; screen dead. It cannot be changed there -- the tail has eight spare bytes and
; it is shared with cdname, which has no picture to keep during an appkey
; call -- so this is its twin, assembled into the one bank that polls.
;
; The launch is identical and has to be: the next sequence number comes from
; the CART'S OWN persisted ACKSEQ + 1, never from a counter in RAM, because a
; console RESET restarts this client without resetting the cartridge and a
; program-local counter would collide with a sequence already answered.
;
; The timeout is the same ~9s in the same FNTMO quanta, so a real stall is
; still reported as the cartridge's error rather than ours: sixteen quanta of
; thirty-four frames is 544 frames, and FNTMO itself is the inner counter --
; FNBEG's value is dead the moment it has been copied into FNCNT, and there is
; no spare byte of zero page in this program to use instead.
NPQUANT EQU     34              ; frames per FNTMO quantum, ~0.57s

NPGO:   lda     FNACKS
        clc
        adc     #1
        bne     NPGO1
        lda     #1              ; 0 is reserved as "never used"
NPGO1:  sta     FNSEQ
        ldx     #FR_SEQ
        jsr     FNRW            ; launching it is this single commit
        lda     FNTMO
        sta     FNCNT
NPGO2:  lda     #NPQUANT
        sta     FNTMO
; ACKSEQ is published LAST, after the reply, its length and the error code are
; all in place, so a match means everything behind it is already readable.
NPGO3:  lda     FNACKS
        cmp     FNSEQ
        beq     NPGO5
        jsr     NFRAME
        dec     FNTMO
        bne     NPGO3
        dec     FNCNT
        bne     NPGO2
        lda     #FNEWAIT
        rts
NPGO5:  lda     FNERR
        rts

; ---------------------------------------------------------------------------
; APICALL -- A = the request (RQ*). One full round trip; on success (A = 0)
; the reply is in the window and stays valid until the next APICALL.
APICALL: sta    CDREQ
        jsr     NSMIN
        jsr     NCLOSE          ; the PREVIOUS request's connection
        jsr     NOPEN
        beq     APOK
        rts                     ; the open failed, and it already closed
APOK:   lda     #0
        sta     CDPRV0
        sta     CDPRV1
        lda     #20             ; the settle loop's bound
        sta     CDTRIES
APSET:  jsr     NSTATUS
        beq     APS1
        jmp     APFAIL
; Enough yet? Two tests, and the first is what the Channel F port added after
; watching a reply settle a third of the way in.
APS1:   lda     CDAVL1          ; avail >= CDMIN, unsigned, through the carry
        cmp     CDMIN1
        bcc     APAGN
        bne     APS2
        lda     CDAVL0
        cmp     CDMIN0
        bcc     APAGN
APS2:   lda     CDAVL0          ; and the same reading twice running
        cmp     CDPRV0
        bne     APAGN
        lda     CDAVL1
        cmp     CDPRV1
        beq     APRD
APAGN:  lda     CDAVL0
        sta     CDPRV0
        lda     CDAVL1
        sta     CDPRV1
        lda     #3              ; ~3 frames between polls
        sta     CDDLY           ; NOT in X or Y: NFRAME is register-counted
APDLY:  jsr     NFRAME          ;   either way and comes back with both gone
        dec     CDDLY
        bne     APDLY
        dec     CDTRIES
        bne     APSET
APFAIL: jsr     NCLOSE          ; failed before the READ: nothing to keep
        lda     #$FF
        rts
APRD:   jsr     NREAD
        beq     APRD1
        jmp     APFAIL
; A validation failure AFTER the read returns nonzero WITHOUT closing: the
; next APICALL's leading NCLOSE retires the connection, and closing here would
; wipe a window the caller may still be rendering the last good reply from.
APRD1:  jmp     VALID8

; CDWAIT -- one frame, roughly. A plain counted delay, used only where the
; screen is blanked anyway: there is no vertical-blank interrupt on this
; console, and the lobby's fetches have no picture to keep.
;
; IT CLOBBERS X AND Y, because it counts in them. Battleship's first caller
; counted its own repeats in Y as well, so every wait ran 256 times over --
; seventy seconds where seventeen milliseconds were meant, which looks exactly
; like a server that never answers.
CDWAIT: ldx     #16             ; ~17ms at 1.19MHz
CDW1:   ldy     #0
CDW2:   dey
        bne     CDW2
        dex
        bne     CDW1
        rts
