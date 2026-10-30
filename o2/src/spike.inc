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

; spk_load: the rows from spk_rows. Clobbers A, r0-r7.
spk_load:
    clr a                       ; the rows
spk_row:
    mov r0,#ROWN
    movx @r0,a
    mov r7,a                    ; its 13 bytes: cards, name, colour
    rl a
    rl a
    add a,r7                    ; 5n
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

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
    include "text.inc"
    include "vdcreset.inc"

s_t0:   db "POT 150 R4  :12",0
s_t1:   db "CHECK  BET  FOLD",0

; ===========================================================================
    org 0C00h
    include "screen.inc"

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
    call spk_spr
spk_grid:
    call grid_flush
spk_l:
    call frame_wait
    jmp spk_l

; spk_spr: every sprite up: a chip on rows 0-6, the marker on row 4, both
; stacks full. Clobbers A, r0-r2, r4, r6.
spk_spr:
    mov r4,#(ROWS+ROW_SPR)&0FFh
    mov r6,#Y_ROW0+CHIPDY
spk_c:
    mov r2,#ROWS>>8
    mov a,r4
    call poke_at
    mov a,r6
    call poke_b
    mov a,r4
    add a,#ROW_TAB
    mov r4,a
    mov a,r6
    add a,#ROW_PITCH
    mov r6,a
    xrl a,#Y_ROW0+CHIPDY+7*ROW_PITCH
    jnz spk_c
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov a,#Y_ROW0+4*ROW_PITCH+MARKDY
    call poke_b
    mov a,#P1_VDC
    outl p1,a
    mov r0,#vdc_spr2_shape
    mov r2,#2*STACK_N
spk_s:
    clr a
    movx @r0,a
    inc r0
    mov a,#7Eh
    movx @r0,a
    inc r0
    djnz r2,spk_s
    mov a,#P1_MAIN
    outl p1,a
    ret

    PAGE_TAIL
