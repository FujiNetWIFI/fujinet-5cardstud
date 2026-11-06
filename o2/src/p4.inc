; p4.inc -- page 4: the table, every frame. The clock; on our turn the move
; menu in T1 (the stick picks, the button plays; or a key: F fold, C check
; or call, B bet, R raise, P post); otherwise the stick moves the spotlight
; over the seats, and the hand's result pages through T1. It reads nothing
; of the reply: page 3 took what it needs (MOVES, MAMT, NMOV ...).

    PAGE_HEAD
    db e_tick&0FFh
e_tick:
    jmp on_tick

on_tick:
    call clock
    call our_go
    jz ot_other
    jmp menu_tick
ot_other:
    call spot_tick
    jmp page_tick

; clock: a second gone: the clock to move ticks down; on our turn the menu
; shows it, and its last five beep. Clobbers A, r0-r2.
clock:
    mov r0,#SECF
    movx a,@r0
    dec a
    movx @r0,a
    jnz ck_x
    mov a,#60
    movx @r0,a
    mov r0,#MOVET
    movx a,@r0
    jz ck_x
    dec a
    movx @r0,a
    mov r2,a
    mov r0,#T1KEY               ; the menu's clock
    mov a,#0FFh
    movx @r0,a
    call our_go
    jz ck_x
    mov a,r2                    ; the last five
    jz ck_x
    add a,#256-6
    jc ck_x
    mov a,#SND_TICK
    jmp beep
ck_x:
    ret

; our_go: A = 1 if it's our move: us to move, a street, moves to pick from.
; Clobbers r0.
our_go:
    mov r0,#ACTV
    movx a,@r0
    jnz og_no
    mov r0,#NMOV
    movx a,@r0
    jz og_no
    mov r0,#ROUND
    movx a,@r0
    jz og_no
    add a,#256-ROUND_SHOW
    jc og_no
    mov a,#1
    ret
og_no:
    clr a
    ret

; menu_tick: our move. Left and right pick (around), the button plays it, a
; key picks one and plays it. Clobbers A, r0-r7.
menu_tick:
    mov r0,#JOYEV
    movx a,@r0
    jb4 mt_fire
    jb2 mt_left
    jb3 mt_right
    mov r0,#KEY
    movx a,@r0
    call key_move
    jnz mt_key
    if AUTOPLAY
    mov r0,#RPCLK               ; a second to think, then the move picked
    movx a,@r0
    inc a
    movx @r0,a
    add a,#256-60
    jc mt_fire
    endif
    jmp mt_draw
mt_key:
    mov r0,#MSEL
    movx @r0,a
    jmp mt_fire
mt_left:
    mov r0,#MSEL
    movx a,@r0
    jnz mt_l1
    mov r0,#NMOV
    movx a,@r0
mt_l1:
    dec a
    jmp mt_pick
mt_right:
    mov r0,#MSEL
    movx a,@r0
    inc a
    mov r1,a
    mov r0,#NMOV
    movx a,@r0
    xrl a,r1
    jz mt_pick                  ; (A = 0: around to the first)
    mov a,r1
mt_pick:
    mov r0,#MSEL
    movx @r0,a
    mov a,#SND_SELECT
    call beep
mt_draw:
    mov r0,#MSEL
    movx a,@r0
    mov r1,a
    mov r0,#T1KEY
    movx a,@r0
    xrl a,r1
    jz mt_x
    mov a,r1
    movx @r0,a
    jmp menu_draw
mt_x:
    ret
mt_fire:
    mov r0,#MSEL                ; the move: its two letters, to go
    movx a,@r0
    rl a
    add a,#MOVES
    mov r0,a
    movx a,@r0
    mov r1,#MOVEC
    movx @r1,a
    inc r0
    inc r1
    movx a,@r0
    movx @r1,a
    mov r0,#ACT
    mov a,#ACT_MOVE
    movx @r0,a
    clr a
    mov r0,#RPCLK
    movx @r0,a
    call menu_line              ; T1: the move, as it goes
    clr a                       ; the menu down: the next state says
    mov r0,#NMOV
    movx @r0,a
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    movx @r0,a
    mov r4,#L_BOT
    call say_line_c
    mov a,#SND_MOVE
    jmp beep

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "text.inc"

s_fold:  db "FOLD",0
s_post:  db "POST",0
s_check: db "CHECK",0
s_call:  db "CALL",0
s_bet:   db "BET",0
s_raise: db "RAISE",0

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
    include "beep.inc"

; menu_line: LINEBUF = the arrow (bit 7: TXCOL2) and the move picked, its
; name and amount. Clobbers A, r0-r7.
menu_line:
    call lb_clear
    mov r0,#LINEBUF
    mov a,#80h|CH_ARROW
    movx @r0,a
    mov r0,#MSEL
    movx a,@r0
    mov r7,a
    rl a
    add a,#MOVES
    mov r0,a
    movx a,@r0
    mov r2,a                    ; its letters
    inc r0
    movx a,@r0
    mov r3,a
    mov r5,#mvnames&0FFh
ml_l:
    mov a,r5
    movp a,@a
    jz ml_put                   ; (none known: the arrow alone)
    xrl a,r2
    jnz ml_n
    inc r5
    mov a,r5
    movp a,@a
    dec r5
    jz ml_hit                   ; (0: any second letter)
    xrl a,r3
    jz ml_hit
ml_n:
    mov a,r5
    add a,#3
    mov r5,a
    jmp ml_l
ml_hit:
    inc r5
    inc r5
    mov a,r5
    movp a,@a
    mov r0,#LINEBUF+1
    call lb_at
    mov a,r7                    ; its amount
    add a,#MAMT
    mov r1,a
    movx a,@r1
    jz ml_put
    mov r4,a
    mov a,#' '
    movx @r0,a
    inc r0
    mov a,r4                    ; up to 3 digits
    mov r6,#'0'-1
ml_100:
    inc r6
    add a,#256-100
    jc ml_100
    add a,#100
    mov r4,a
    mov a,r6
    xrl a,#'0'
    jz ml_t
    mov a,r6
    movx @r0,a
    inc r0
ml_t:
    mov a,r4
    mov r6,#'0'-1
ml_10:
    inc r6
    add a,#256-10
    jc ml_10
    add a,#10
    mov r4,a
    mov a,r6
    xrl a,#'0'
    jz ml_u
    mov a,r6
    movx @r0,a
    inc r0
ml_u:
    mov a,r4
    add a,#'0'
    movx @r0,a
ml_put:
    ret

mvnames:                        ; first letter, second (0: any), its name
    db 'f', 0, s_fold&0FFh
    db 'b', 'b', s_post&0FFh
    db 'c', 'h', s_check&0FFh
    db 'c', 'a', s_call&0FFh
    db 'b', 0, s_bet&0FFh
    db 'r', 0, s_raise&0FFh
    db 0
    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
; menu_draw: T1 = our move menu: the arrow, the move picked and its amount,
; which of how many, and the clock. Clobbers A, r0-r7.
menu_draw:
    call menu_line
    mov r0,#NMOV                ; which of how many
    movx a,@r0
    mov r6,a
    dec a
    jz md_clk
    mov r0,#MSEL
    movx a,@r0
    add a,#'1'
    mov r0,#LINEBUF+10
    movx @r0,a
    inc r0
    mov a,#'/'
    movx @r0,a
    inc r0
    mov a,r6
    add a,#'0'
    movx @r0,a
md_clk:
    mov r0,#MOVET               ; the clock, 2 digits
    movx a,@r0
    mov r6,#'0'-1
md_10:
    inc r6
    add a,#256-10
    jc md_10
    add a,#'0'+10
    mov r0,#LINEBUF+15
    movx @r0,a
    dec r0
    mov a,r6
    movx @r0,a
    dec r0
    mov a,#':'
    movx @r0,a
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    mov a,#C_TAN
    movx @r0,a
    mov r4,#L_BOT
    jmp say_line_c

; key_move: A = the move (1-based) key A asks for, of ours; 0 none.
; Clobbers A, r0-r3.
key_move:
    mov r2,#'f'                 ; the key's move's first letter, and its
    mov r3,#0                   ; second (0: any)
    xrl a,#K_F
    jz km_find
    mov r2,#'c'
    xrl a,#K_F!K_C
    jz km_find
    mov r2,#'b'
    xrl a,#K_C!K_B
    jz km_find
    mov r2,#'r'
    xrl a,#K_B!K_R
    jz km_find
    mov r2,#'b'
    mov r3,#'b'
    xrl a,#K_R!K_P
    jz km_find
    clr a
    ret
km_find:
    mov r0,#NMOV
    movx a,@r0
    mov r1,a
    mov r0,#MOVES
km_l:
    movx a,@r0
    xrl a,r2
    jnz km_n
    mov a,r3
    jz km_yes
    inc r0
    movx a,@r0
    dec r0
    xrl a,r3
    jz km_yes
km_n:
    inc r0
    inc r0
    djnz r1,km_l
    clr a
    ret
km_yes:
    mov a,r0                    ; (r0 - MOVES) / 2 + 1
    add a,#256-MOVES
    rr a
    inc a
    ret

    if $ > 0D00h
    error "page $C has run out"
    endif

; ===========================================================================
    org 0D00h
; spot_tick: up and down move the spotlight over the seats: its marker at
; once, and a state asked for to describe it in T1. Clobbers A, r0-r7.
spot_tick:
    mov r0,#JOYEV
    movx a,@r0
    jb0 sp_up
    jb1 sp_down
    ret
sp_up:
    mov r0,#SPOT
    movx a,@r0
    jnz sp_u1
    mov r0,#NSEAT
    movx a,@r0
sp_u1:
    dec a
    jmp sp_set
sp_down:
    mov r0,#SPOT
    movx a,@r0
    inc a
    mov r1,a
    mov r0,#NSEAT
    movx a,@r0
    xrl a,r1
    jz sp_set                   ; (A = 0: around to us)
    mov a,r1
sp_set:
    mov r0,#SPOT
    movx @r0,a
    mov r1,#1                   ; seven opponents: the rows follow it
    xrl a,#1
    jz sp_off
    mov r1,#2
    xrl a,#1!7
    jnz sp_go
sp_off:
    mov r0,#OPPOFF
    movx a,@r0
    xrl a,r1
    jz sp_go
    mov a,r1
    movx @r0,a
    mov r0,#RSIG                ; every row again
    mov r1,#NROWS
    mov a,#0FFh
sp_rs:
    movx @r0,a
    inc r0
    djnz r1,sp_rs
sp_go:
    call spot_mark
    clr a
    mov r0,#POLLW
    movx @r0,a
    mov a,#SND_MOVE
    jmp beep

; page_tick: waiting, or the hand over: lastResult's next page every second
; and a half (page 3 draws it). Clobbers A, r0.
page_tick:
    mov r0,#ROUND
    movx a,@r0
    jz pt_go
    xrl a,#ROUND_SHOW
    jnz pt_x
pt_go:
    mov r0,#RPCLK
    movx a,@r0
    inc a
    movx @r0,a
    add a,#256-90
    jnc pt_x
    clr a
    movx @r0,a
    mov r0,#RNEXT
    movx a,@r0
    mov r0,#RPAGE
    movx @r0,a
pt_x:
    ret

    include "sprites.inc"

    if $ > 0E00h
    error "page $D has run out"
    endif

    PAGE_TAIL
