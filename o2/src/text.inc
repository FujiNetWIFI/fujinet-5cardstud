; text.inc -- the screens' text lines: say, say_line, lb_str. The includer
; puts it at the start of a page and that page's strings after it: say reads
; them with MOVP.

; say: line r4 (L_*) = the string at offset A of this page, white.
; Clobbers A, r0-r7.
say:
    mov r6,a
    call lb_clear
    mov a,r6
    call lb_str
    ; fall into say_line

; say_line: LINEBUF onto line r4, white. say_line_c: in TXCOL / TXCOL2 as
; they are. Clobbers A, r0-r7.
say_line:
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    movx @r0,a
say_line_c:
    mov a,r4
    rl a
    add a,r4                    ; *3
    add a,#lines&0FFh
    mov r6,a
    movp a,@a
    mov r3,a                    ; the table's low byte
    inc r6
    mov a,r6
    movp a,@a
    mov r2,a                    ; its page
    inc r6
    mov a,r6
    movp a,@a
    mov r4,a                    ; its Y
    jmp tx_build

; lb_str: the NUL-terminated string at offset A of this page into LINEBUF;
; lb_at: into LINEBUF from r0. Clobbers A, r0, r1.
lb_str:
    mov r0,#LINEBUF
lb_at:
    mov r1,a
lbs_l:
    mov a,r1
    movp a,@a
    jz lbs_x
    movx @r0,a
    inc r0
    inc r1
    jmp lbs_l
lbs_x:
    ret

; say_row: row r4 (0-7) = the string at offset A of this page, white.
; Clobbers A, r0-r7.
say_row:
    mov r6,a
    call lb_clear
    mov a,r6
    call lb_str
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    movx @r0,a
    ; fall into row_line

; row_line: LINEBUF's first NCOLS characters onto row r4 of a text screen,
; in TXCOL (TXCOL2 for a character with bit 7 set). Clobbers A, r0-r7.
row_line:
    mov a,r4                    ; the bias: -(Y/2), Y = Y_ROW0 + 24r
    rl a
    rl a
    rl a
    mov r5,a                    ; 8r
    rl a
    add a,r5                    ; 24r
    add a,#Y_ROW0
    rr a
    cpl a
    inc a
    mov r5,a
    mov a,r4                    ; the row's table: ROWS + r * ROW_TAB
    mov r6,a
    mov a,#ROWS&0FFh
    inc r6
    jmp rl_t
rl_m:
    add a,#ROW_TAB
rl_t:
    djnz r6,rl_m
    mov r2,#ROWS>>8
    call poke_at
    mov r6,#LINEBUF
rl_l:
    mov a,r6
    mov r0,a
    movx a,@r0
    mov r7,a
    anl a,#7Fh
    call gptr
    mov a,r3
    call poke_b                 ; pointer, low byte
    mov r0,#TXCOL
    mov a,r7
    jb7 rl_c2
    jmp rl_c1
rl_c2:
    inc r0
rl_c1:
    movx a,@r0
    rl a
    orl a,r1
    call poke_b                 ; colour and pointer bit 8
    inc r6
    mov a,r6
    xrl a,#LINEBUF+NCOLS
    jnz rl_l
    ret

; The lines: table (low byte, page) and Y: T0 at the top, T1 at the bottom.
lines:
    db T0&0FFh, T0>>8, Y_T0
    db T1&0FFh, T1>>8, Y_T1
L_TOP   equ 0
L_BOT   equ 1
