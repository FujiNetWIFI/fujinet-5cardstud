; pad.asm -- note at every frame interrupt whether button 2 is down.
;
; The main loop spends most of its time inside HTTP round trips, and a tap of
; button 2 that starts and ends inside one would never be polled. Installed
; with add_raster_int() by src/sms/input.c; crt0's handler saves AF, BC, DE
; and HL around it.

    SECTION code_user

    PUBLIC  _padIrq
    PUBLIC  _b2Latch

_padIrq:
    in      a,($DC)
    and     $20                     ; button 2, active low
    ret     nz
    ld      a,1
    ld      (_b2Latch),a
    ret

    SECTION bss_user

_b2Latch:
    defb    0
