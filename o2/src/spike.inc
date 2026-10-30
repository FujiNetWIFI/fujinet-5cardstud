; spike.inc -- page 0 of a SPIKE build: the table engine's test pattern.
; The worst case on every row -- a name and five cards face up, or a card
; face down -- and our hand, T0 and T1. No network. For measuring a band's
; slack: PADN pads a row band (build.sh), and tools/rowcheck.py counts the
; frames that lost a line.

    PAGE_HEAD
    db e_boot&0FFh
e_boot:
    jmp boot

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "input.inc"
    include "vdcreset.inc"

; ===========================================================================
    org 0B00h
    include "text.inc"

s_t0:   db "POT 150 R4  :12",0
s_t1:   db "CHECK  BET  FOLD",0

; ===========================================================================
    org 0C00h
    include "screen.inc"

; spk_load: the rows from spk_rows. Clobbers A, r0-r7.
spk_load:
    clr a                       ; the rows
spk_row:
    mov r0,#ROWN
    movx @r0,a
    mov r7,a                    ; its 13 bytes: cards, name, colour
    rl a
    rl a
    add a,r7
    mov r6,a                    ; 5n
    rl a                        ; 10n
    add a,r7
    add a,r7
    add a,r7                    ; 13n
    add a,#spk_rows&0FFh
    mov r1,a
    mov r0,#CARDS               ; (CARDS, ROWNAME, ROWNC are in a row)
    mov r2,#13
spk_cp:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r2,spk_cp
    call row_put
    mov r0,#ROWN
    movx a,@r0
    inc a
    xrl a,#NROWS
    jz spk_x
    xrl a,#NROWS
    jmp spk_row
spk_x:
    ret

    if CARDS+2*NCARDS<>ROWNAME
    error "spike: CARDS, ROWNAME and ROWNC have to be in a row"
    endif
    if ROWNAME+2<>ROWNC
    error "spike: CARDS, ROWNAME and ROWNC have to be in a row"
    endif

spk_rows:                       ; cards, name, colour
    db "ASKHQDJCTS", "MO", C_WHITE
    db "2H3H4H5H6H", "CL", C_WHITE
    db "??9S9D8C8H", "KI", C_WHITE
    db "TCTHTSTD7S", "JI", C_WHITE
    db "AD2D3C4S5H", "HU", C_TAN
    db "??KCKDKSQH", "BO", C_DGREY
    db "ASKSQSJSTS", "YO", C_CYAN
    db "ASKSQSJSTS", "U ", C_CYAN

    if $ > 0D00h
    error "page $C has run out"
    endif

; ===========================================================================
    org 0D00h
    include "table.inc"

; ===========================================================================
    org 0E60h

boot:
    call vdc_reset
    call scr_table
    clr a
    mov t,a
    strt cnt
    en tcnti
    en i
    mov r4,#L_TOP
    mov a,#s_t0&0FFh
    call say
    mov r4,#L_BOT
    mov a,#s_t1&0FFh
    call say
    call spk_load
spk_grid:
    call grid_flush
spk_l:
    call frame_wait
    jmp spk_l

    PAGE_TAIL
