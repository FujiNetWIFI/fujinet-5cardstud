; screen.inc -- the screens' band list, and in a page that defines TEXTSCR
; the text screen (table.inc has the table's). One 256-byte page: the
; template with the code that copies it.

; spr_hide: the four sprites below the screen, and every row's chip.
; Clobbers A, r0-r2, r4, r6.
spr_hide:
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov r6,#4
sh_l:
    mov a,#SPR_OFF
    call poke_b
    clr a
    call poke_b
    clr a
    call poke_b
    djnz r6,sh_l
    mov r4,#(ROWS+ROW_SPR)&0FFh
sh_r:
    mov r2,#ROWS>>8
    mov a,r4
    call poke_at
    mov a,#SPR_OFF
    call poke_b
    mov a,r4
    add a,#ROW_TAB
    mov r4,a
    xrl a,#(ROWS+ROW_SPR+NROWS*ROW_TAB)&0FFh
    jnz sh_r
    ret

    ifndef TEXTSCR
; spr_table: the table's sprites, hidden: the spotlight's marker and a bet's
; chip, their shapes and places (spot_mark and the rows give their Ys), and
; T0's two stacks, empty (t0_draw fills them). Clobbers A, r0-r2, r4, r6.
spr_table:
    call spr_hide
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov r4,#spr_t&0FFh
    mov r6,#12
    call copy_rom
    mov a,#P1_VDC
    outl p1,a
    mov r0,#vdc_spr0_shape
    mov r1,#spr_s&0FFh
    mov r2,#16
spt_s:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r2,spt_s
    clr a
    mov r2,#16
spt_z:
    movx @r0,a
    inc r0
    djnz r2,spt_z
    mov a,#P1_MAIN
    outl p1,a
    ret
spr_t:                          ; 4 x (Y, X, attribute: colour << 3)
    db SPR_OFF, MARKX, C_WHITE<<3   ; the spotlight's marker
    db SPR_OFF, CHIPX, C_RED<<3     ; a bet's chip, down the rows
    db STACKY, POTX, C_RED<<3       ; the pot's stack
    db STACKY, OURX, C_WHITE<<3     ; our purse's
spr_s:                          ; their shapes, bit 0 leftmost
    db 03h, 0Fh, 3Fh, 0Fh, 03h, 0, 0, 0     ; a pointer
    db 3Ch, 66h, 5Ah, 66h, 3Ch, 0, 0, 0     ; a chip, face on
    endif

; copy_rom: r6 bytes from offset r4 of this page to the POKE pointer.
copy_rom:
    mov a,r4
    movp a,@a
    call poke_b
    inc r4
    djnz r6,copy_rom
    ret

; poke_n: r6 copies of A at the POKE pointer. Clobbers r0, r6.
poke_n:
    call poke_b
    djnz r6,poke_n
    ret

; bands: the colour and the band list from template r4 of this page.
; Clobbers A, r0-r2, r4, r6.
bands:
    mov r2,#SCRCOL>>8
    mov a,#SCRCOL&0FFh
    call poke_at
    mov r6,#2+NBANDS*3
    jmp copy_rom

    ifdef TEXTSCR
; scr_text: a text screen: eight blank rows of text between T0 and T1, on
; black, the grid empty, the sprites hidden. Rows first, then the band
; list: a frame between them draws the old screen. Clobbers A, r0-r7.
scr_text:
    call lb_clear
    mov r4,#0
st_r:
    mov r0,#TMP0                ; (row_line keeps no register)
    mov a,r4
    movx @r0,a
    call row_line
    mov r0,#TMP0
    movx a,@r0
    inc a
    mov r4,a
    xrl a,#NROWS
    jnz st_r
    mov r2,#GRIDTAB>>8          ; the grid empty
    mov a,#GRIDTAB&0FFh
    call poke_at
    mov r6,#10
    clr a
    call poke_n
    mov r4,#scr_t&0FFh
    call bands
    mov r2,#SCRCOL>>8           ; on black
    mov a,#SCRCOL&0FFh
    call poke_at
    mov a,#COL_TEXT
    call poke_b
    call spr_hide
    mov r1,#MODE
    mov a,#MODE_SETUP|MODE_TEXT
    mov @r1,a
    ; fall into scr_lines

; scr_lines: T0 and T1 blank. Clobbers A, r0-r7.
scr_lines:
    call lb_clear
    mov r4,#L_TOP
    call say_line
    call lb_clear
    mov r4,#L_BOT
    jmp say_line
    endif

; bands_table: the table's colour and band list. Clobbers A, r0-r2, r4, r6.
bands_table:
    mov r4,#scr_t&0FFh
    jmp bands

; The bands' lines. Row r's characters are at Y_ROW0 + 24r; the band before
; it starts when row r-1's have ended, LEAD lines early (the interrupt takes
; about a line to reach its first write). Every screen is this one: a text
; screen changes the colour and the singles' X.
BL_TOP  equ 1                   ; row 0 and the grid, before T0 shows
BL_GRID equ Y_T0+13-LEAD        ; T0 has ended: the cards on (h_grid_b
                                ; pads its write into the blank)
BL_ROW1 equ Y_ROW0+ROW_PITCH-10-LEAD
BL_T1   equ Y_HAND2+14-LEAD
scr_t:
    db COL_TABLE
    db 256-BL_TOP
    db J_TOP, (ROWS&0FFh)+ROW_TAB, 256-(BL_GRID-BL_TOP)
    db J_GRID, 0, 256-(BL_ROW1-BL_GRID)
K   set 2
    rept 6                      ; rows 1-6, each staging the next (its table)
    db J_ROW, (ROWS&0FFh)+K*ROW_TAB, 256-ROW_PITCH
K   set K+1
    endm
    db J_ROW2, 0FFh, 256-(BL_T1-(BL_ROW1+6*ROW_PITCH))  ; row 7
    db J_TXT, T1&0FFh, 0        ; T1: the last
