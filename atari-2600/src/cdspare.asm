; cdspare.asm -- banks 5 and 6: nothing, safely.
;
; Assembled ONCE and concatenated twice; build.sh does that rather than this
; file existing in two copies to be edited in one of them.
;
; A cartridge image has to be (N+1) * 2048 bytes with N in {1,3,7,15} -- MAME's
; vcs_cart_slot_device::call_load() accepts no other size -- and this client
; needs five banks. Seven is the size that fits, so two exist whether or not
; they are wanted.
;
; They are not left as zeros. A zero byte is BRK, and a stray bank switch
; followed by a fetch would run $00 $00 $00 forever with the vectors pointing
; back into a cold start it never reaches. Each spare bank instead does the
; one sensible thing: go to bank 0 and start again.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "cddefs.inc"

        ORG     $1000

        lda     #2
        sta     VBLANK
        lda     #ENCOLD
        sta     CDENT
        lda     #BANKLOB
        jmp     CDGOTO

        END
