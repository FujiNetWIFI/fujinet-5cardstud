; table.inc -- the table screen: eight rows of cards between T0 and T1, on
; the felt. Rows 0-5 are the opponents, a card a grid cell; rows 6 and 7 are
; our hand, a card two cells tall with its rank in the top row and its suit
; in the bottom one. A row's table is the fixed half's page $6 (ROWS), its
; cards the grid's cells (GRIDTAB, page $7), both written through POKE.
; One 256-byte page: these routines read its tables with MOVP. Main program.

; The tables first: row_put reads them with MOVP, and the routines run
; past the page.
rowb:                           ; each row's bias, -(Y/2)
K   set 0
    rept NROWS
    db (256-(Y_ROW0+K*ROW_PITCH)/2)&0FFh
K   set K+1
    endm
rowt:                           ; each row's table in ROWS (page $6)
K   set 0
    rept NROWS
    db (ROWS&0FFh)+K*ROW_TAB
K   set K+1
    endm

; scr_table: the table screen -- every row empty, T0 and T1 blank, the
; sprites hidden. Rows first, then the band list: a frame between them draws
; the old screen. Clobbers A, r0-r7.
scr_table:
    mov r0,#CARDS               ; no cards ...
    mov r2,#2*NCARDS
    clr a
stb_c:
    movx @r0,a
    inc r0
    djnz r2,stb_c
    mov a,#' '                  ; ... and no names (ROWNAME follows)
    movx @r0,a
    inc r0
    movx @r0,a
    clr a
    mov r0,#ROWN
stb_r:
    movx @r0,a
    call row_put
    mov r0,#ROWN
    movx a,@r0
    inc a
    xrl a,#NROWS
    jz stb_b
    xrl a,#NROWS
    jmp stb_r
stb_b:
    call grid_flush
    call bands_table            ; the colour and the band list
    call spr_hide
    mov r1,#MODE
    mov a,#MODE_SETUP
    mov @r1,a
    call lb_clear
    mov r4,#L_TOP
    call say_line
    call lb_clear
    mov r4,#L_BOT
    jmp say_line

; row_put: row ROWN (0-7) = the hand at CARDS (5 x rank, suit; wire ASCII:
; '2'-'9' 't' 'j' 'q' 'k' 'a', 's' 'h' 'd' 'c'; "??" face down; NUL or space
; no card) and the name at ROWNAME in colour ROWNC. Its cards' cells go to
; GRIDS (grid_flush puts them on the screen: h_top writes the grid every
; frame). Clobbers A, r0-r7.
row_put:
    mov r0,#ROWN
    movx a,@r0
    mov r7,a                    ; the row
    add a,#rowb&0FFh
    movp a,@a
    mov r5,a                    ; its bias: -(Y/2)
    mov a,r7
    add a,#rowt&0FFh
    movp a,@a
    mov r2,#ROWS>>8
    call poke_at
    mov r6,#0                   ; the name
rp_name:
    mov a,r6
    add a,#ROWNAME
    mov r0,a
    movx a,@r0
    call gptr
    mov r0,#ROWNC
    movx a,@r0
    call put_cell
    inc r6
    mov a,r6
    xrl a,#NAMEW
    jnz rp_name
    mov r6,#0                   ; the cards
rp_card:
    call card_codes
    mov a,r7                    ; the hand's rows show half a card each
    xrl a,#6
    jz rp_top
    xrl a,#7!6
    jnz rp_both
    mov r0,#SCR0+1              ; the bottom: the suit, under the rank's place
    movx a,@r0
    jb7 rp_both                 ; (a card face down shows whole)
    mov r0,#SCR0
    mov a,#' '
    movx @r0,a
    jmp rp_both
rp_top:
    mov r0,#SCR0+1              ; the top: the rank alone
    movx a,@r0
    jb7 rp_both
    mov a,#' '
    movx @r0,a
rp_both:
    mov r0,#SCR0
    movx a,@r0
    call gptr
    mov r0,#SCR0+2
    movx a,@r0
    call put_cell
    mov r0,#SCR0+1
    movx a,@r0
    anl a,#7Fh
    call gptr
    mov r0,#SCR0+2
    movx a,@r0
    call put_cell
    inc r6
    mov a,r6
    xrl a,#NCARDS
    jnz rp_card
    ret

; card_codes: card r6 of CARDS -> SCR0 its rank's character code, SCR0+1 its
; suit's (bit 7 set: a card face down), SCR0+2 their colour; and its cell in
; row r7 set or cleared in GRIDS. Clobbers A, r0-r4.
card_codes:
    mov a,r6
    rl a
    add a,#CARDS
    mov r0,a
    movx a,@r0
    orl a,#20h                  ; lowercase (digits, '?' and ' ' have it)
    mov r4,a                    ; the rank
    inc r0
    movx a,@r0
    orl a,#20h
    mov r2,a                    ; the suit
    mov a,r4
    xrl a,#' '
    jz cc_none
    xrl a,#' '!'?'
    jz cc_back
    xrl a,#'?'!'t'
    jnz cc_rank
    mov r4,#CH_TEN              ; ten: "10" in one cell
cc_rank:
    mov r3,#C_DGREY             ; spades and clubs; hearts and diamonds red
    mov a,r2
    xrl a,#'h'
    jz cc_red
    xrl a,#'h'!'d'
    jnz cc_put
cc_red:
    mov r3,#C_RED
    jmp cc_put
cc_back:
    mov r4,#CH_BLOCK            ; the back: a blue block in both places
    mov r2,#80h|CH_BLOCK
    mov r3,#C_BLUE
cc_put:
    mov r1,#1                   ; a card: its cell on
    jmp cc_set
cc_none:
    mov r4,#' '
    mov r2,#' '
    mov r3,#C_DGREY             ; (a blank's colour still goes in its
    mov r1,#0                   ; control byte, beside pointer bit 8)
cc_set:
    mov r0,#SCR0
    mov a,r4
    movx @r0,a
    inc r0
    mov a,r2
    movx @r0,a
    inc r0
    mov a,r3
    movx @r0,a
    mov a,r6                    ; GRIDS[card]: bit (row)
    add a,#GRIDS
    mov r0,a
    mov a,r7                    ; its bit: 1 << row
    mov r2,a
    inc r2
    mov a,#80h
cc_bit:
    rl a
    djnz r2,cc_bit
    mov r2,a
    cpl a
    mov r3,a
    movx a,@r0
    anl a,r3
    mov r3,a
    mov a,r1
    jz cc_x
    mov a,r3
    orl a,r2
    mov r3,a
cc_x:
    mov a,r3
    movx @r0,a
    ret

; put_cell: one single's (pointer, control): pointer r3 (gptr's), colour A,
; pointer bit 8 r1. Clobbers A, r0, r4.
put_cell:
    mov r4,a
    mov a,r3
    call poke_b
    mov a,r4
    rl a
    orl a,r1
    jmp poke_b

; grid_flush: GRIDS onto the grid (GRIDTAB, page $7): a card in every other
; column from CARD_COL0. h_top writes it. Clobbers A, r0-r2, r6.
grid_flush:
    mov r2,#GRIDTAB>>8
    mov a,#GRIDTAB&0FFh
    call poke_at
    mov r6,#0
gf_l:
    mov a,r6
    xrl a,#CARD_COL0
    anl a,#1
    jnz gf_none
    mov a,r6
    rr a
    anl a,#7
    add a,#GRIDS
    mov r0,a
    movx a,@r0
    jmp gf_put
gf_none:
    clr a
gf_put:
    call poke_b
    inc r6
    mov a,r6
    xrl a,#10
    jnz gf_l
    ret
