; vdp.asm -- VDP port access with rules C cannot promise, the tile set
; expander, which sccz80 would take seconds over, and a correct clear_vram.
;
; The control port takes an address in two writes, and the frame interrupt's
; status read between them would reset the latch and land the second byte as
; an address low byte: vdp_addr keeps the pair under DI. The data port wants
; 26 T-states between VRAM writes during active display, as crt0's RST $18
; spaces them.

    SECTION code_user

    PUBLIC  _vdp_addr
    PUBLIC  _vdp_word
    PUBLIC  _vdp_byte

; void vdp_addr(unsigned int cmd) __z88dk_fastcall -- L first, then H
_vdp_addr:
    ld      a,l
    di
    out     ($BF),a
    ld      a,h
    out     ($BF),a
    ei
    ret

; void vdp_word(unsigned int w) __z88dk_fastcall -- a name table entry, L first
_vdp_word:
    ld      a,l
    out     ($BE),a                 ; 11
    ld      a,h                     ; 4
    sub     0                       ; 7
    nop                             ; 4 = 26
    out     ($BE),a
    ret

; void vdp_byte(unsigned char b) __z88dk_fastcall
_vdp_byte:
    ld      a,l
    out     ($BE),a
    ret

; void vdp_tiles(const unsigned char *runs) __z88dk_fastcall
;
; Expand src/sms/tileset.c into VRAM, display off. A run is { tile lo, tile
; hi, count }, a tile two bytes of bitplane truth tables then 8 rows of
; (lo, hi) 2bpp. A plane's byte is the OR of the masks of the 2bpp codes its
; truth table selects; the four masks live in the alternate B'C'D'E'.

    PUBLIC  _vdp_tiles

_vdp_tiles:
    ld      e,(hl)
    inc     hl
    ld      d,(hl)
    inc     hl
    ld      a,(hl)
    inc     hl
    or      a
    ret     z
    ld      b,a                     ; tiles in this run
    push    hl
    ex      de,hl
    add     hl,hl
    add     hl,hl
    add     hl,hl
    add     hl,hl
    add     hl,hl                   ; tile * 32
    ld      a,l
    di
    out     ($BF),a
    ld      a,h
    or      $40                     ; VRAM write
    out     ($BF),a
    ei
    pop     hl
vt_tile:
    ld      a,(hl)
    inc     hl
    ld      (vt_t01),a
    ld      a,(hl)
    inc     hl
    ld      (vt_t23),a
    ld      c,8
vt_row:
    ld      d,(hl)                  ; lo
    inc     hl
    ld      e,(hl)                  ; hi
    inc     hl
    ld      a,d
    or      e
    cpl
    exx
    ld      b,a                     ; code 0: neither bit
    exx
    ld      a,e
    cpl
    and     d
    exx
    ld      c,a                     ; code 1: lo only
    exx
    ld      a,d
    cpl
    and     e
    exx
    ld      d,a                     ; code 2: hi only
    exx
    ld      a,d
    and     e
    exx
    ld      e,a                     ; code 3: both
    ld      a,(vt_t01)
    call    vt_plane
    ld      a,(vt_t01)
    rrca
    rrca
    rrca
    rrca
    call    vt_plane
    ld      a,(vt_t23)
    call    vt_plane
    ld      a,(vt_t23)
    rrca
    rrca
    rrca
    rrca
    call    vt_plane
    exx
    dec     c
    jr      nz,vt_row
    djnz    vt_tile
    jr      _vdp_tiles

; A = truth table in its low nibble; alternate set active.
vt_plane:
    ld      h,a
    xor     a
    rr      h
    jr      nc,vt_p1
    or      b
vt_p1:
    rr      h
    jr      nc,vt_p2
    or      c
vt_p2:
    rr      h
    jr      nc,vt_p3
    or      d
vt_p3:
    rr      h
    jr      nc,vt_p4
    or      e
vt_p4:
    out     ($BE),a
    ret

; crt0 clears VRAM before main. z88dk's clear_vram starts its counter at
; $4000 with L = 0 and so writes the 16K four times over -- most of a second
; of black screen; defining the symbol here keeps that module out of the
; link. The display is still off, so the writes need no spacing.

    PUBLIC  clear_vram
    PUBLIC  _clear_vram

clear_vram:
_clear_vram:
    xor     a
    out     ($BF),a
    ld      a,$40                   ; VRAM write from $0000
    out     ($BF),a
    xor     a
    ld      c,$40                   ; 64 pages of 256
cv_page:
    ld      b,a
cv_byte:
    out     ($BE),a
    djnz    cv_byte
    dec     c
    jr      nz,cv_page
    ret

    SECTION bss_user

vt_t01:
    defb    0
vt_t23:
    defb    0
