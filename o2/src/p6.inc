; p6.inc -- page 6: the table, when a state has come, after page 3 (the
; same pass of the main loop, so the reply is still the state's): T0 (the
; pot, our purse, and their stacks of chips), T1 (whose turn and what they
; did, or the hand's result; on our turn the menu is page 4's), the sounds,
; and the chips of the street's bets. Then the state is the screen's:
; LROUND, LACTV.

    PAGE_HEAD
    db e_state2&0FFh
e_state2:
    jmp on_state2

on_state2:
    call t0_draw
    call t1_state
    call cues
    mov r0,#ROWSNEW             ; a bet comes with a row redrawn: a turn
    movx a,@r0                  ; passed, a street dealt, a scroll
    jz os2_c
    call chips
os2_c:
    mov r0,#ROUND
    movx a,@r0
    mov r0,#LROUND
    movx @r0,a
    mov r0,#ACTV
    movx a,@r0
    mov r0,#LACTV
    movx @r0,a
    ret

; cues: the sounds for this state, against the last: a street dealt, the
; hand won, our turn come, chips in the pot. None for the first.
; Clobbers A, r0-r7.
cues:
    mov r3,#0                   ; the pot's low byte
    mov r2,#GAME_POT
    call st_byte
    mov r5,a
    mov r0,#LPOT
    movx a,@r0
    mov r6,a
    mov a,r5
    movx @r0,a
    mov r0,#LROUND
    movx a,@r0
    mov r4,a
    inc a
    jz cu_x                     ; the first state
    mov r0,#ROUND
    movx a,@r0
    xrl a,r4
    jz cu_turn
    movx a,@r0
    jz cu_x                     ; waiting
    xrl a,#ROUND_SHOW
    jz cu_show
    mov a,#SND_DEAL
    jmp beep
cu_show:
    call won_it
    jz cu_x
    mov a,#SND_WIN
    jmp beep
cu_turn:
    mov r0,#ACTV
    movx a,@r0
    jnz cu_pot
    mov r0,#LACTV
    movx a,@r0
    jz cu_pot
    mov a,#SND_TURN
    jmp beep
cu_pot:
    mov a,r5
    xrl a,r6
    jz cu_x
    mov a,#SND_CHIPS
    jmp beep
cu_x:
    ret

; won_it: A = 1 if lastResult starts with our name. Clobbers A, r0-r3, r6.
won_it:
    mov r3,#0
    mov r2,#GAME_LASTRESULT
    mov r1,#SCNAME
wi_l:
    mov a,r1
    mov r0,a
    movx a,@r0
    jz wi_yes
    orl a,#20h
    mov r4,a
    call st_byte
    orl a,#20h
    xrl a,r4
    jnz wi_no
    inc r1
    inc r2
    jmp wi_l
wi_yes:
    mov a,#1
    ret
wi_no:
    clr a
    ret


; our_round: A = 1 if ROUND is a street (1-4), else 0. Clobbers r0.
our_round:
    mov r0,#ROUND
    movx a,@r0
    jz or_no
    add a,#256-ROUND_SHOW
    jc or_no
    mov a,#1
    ret
or_no:
    clr a
    ret

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "text.inc"

s_pot:  db "POT",0

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
    include "stbyte.inc"
    include "beep.inc"

; seat_base: r3:r2 = seat A's record in the reply. Clobbers A, r6.
seat_base:
    mov r6,a                    ; 154 + 33A (A < 8)
    swap a
    rl a
    add a,r6
    add a,#GAME_SEATS
    mov r2,a
    clr a
    addc a,#0
    mov r3,a
    ret

; put_u16: the number r5:r4 (hi:lo), in decimal, into LINEBUF at r1, no
; leading zeros; r1 advanced. Clobbers A, r0, r2, r3, r6, r7.
put_u16:
    mov r7,#0                   ; a digit has gone out
    mov r2,#10000&0FFh
    mov r3,#10000>>8
    call pu_dig
    mov r2,#1000&0FFh
    mov r3,#1000>>8
    call pu_dig
    mov r2,#100
    mov r3,#0
    call pu_dig
    mov r2,#10
    call pu_dig
    mov r7,#1                   ; the units: always
    mov r2,#1
pu_dig:                         ; r5:r4 / r3:r2 -> a digit, r5:r4 the rest
    mov r6,#'0'
pd_l:
    clr c                       ; r5:r4 - r3:r2 = r5:r4 + ~r3:~r2 + 1
    cpl c
    mov a,r2
    cpl a
    addc a,r4
    mov r0,a
    mov a,r3
    cpl a
    addc a,r5                   ; C: no borrow, it was at least r3:r2
    jnc pd_e
    mov r5,a
    mov a,r0
    mov r4,a
    inc r6
    jmp pd_l
pd_e:
    mov a,r6
    xrl a,#'0'
    orl a,r7
    jz pd_x
    mov r7,#1
    mov a,r6
    movx @r1,a
    inc r1
pd_x:
    ret

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
; t1_state: T1 for this state: on our turn page 4's menu (drawn afresh);
; waiting for players, or the hand over, its result; else the spotlight's
; seat. Clobbers A, r0-r7.
t1_state:
    mov r0,#ROUND               ; a new round: the result from its start
    movx a,@r0
    mov r4,a
    mov r0,#LROUND
    movx a,@r0
    xrl a,r4
    jz t1_go
    clr a
    mov r0,#RPAGE
    movx @r0,a
    mov r0,#RPCLK
    movx @r0,a
t1_go:
    call our_round
    jnz t1_st
    jmp lr_draw
t1_st:
    mov r0,#NMOV
    movx a,@r0
    jz seat_line
    mov r0,#T1KEY
    mov a,#0FFh
    movx @r0,a
    ret

; seat_line: T1 = the spotlight's seat: its name and what it did last, tan
; if it is to move. Clobbers A, r0-r7.
seat_line:
    call lb_clear
    mov r0,#SPOT
    movx a,@r0
    mov r5,a
    call seat_base
    mov r1,#LINEBUF             ; its name, 8
    mov r4,#8
    call sl_copy
    mov a,r2                    ; its move, 7: 12 past its name
    add a,#SE_MOVE-8
    mov r2,a
    clr a
    addc a,r3
    mov r3,a
    mov r1,#LINEBUF+9
    mov r4,#7
    call sl_copy
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    mov r0,#ACTV
    movx a,@r0
    xrl a,r5
    jnz sl_put
    mov r0,#TXCOL
    mov a,#C_TAN
    movx @r0,a
sl_put:
    mov r0,#TXCOL               ; TXCOL2 the same
    movx a,@r0
    inc r0
    movx @r0,a
    mov r4,#L_BOT
    jmp say_line_c
sl_copy:                        ; r4 bytes from r3:r2 to r1, NUL a space
    call st_byte
    jnz slc_1
    mov a,#' '
slc_1:
    movx @r1,a
    inc r1
    inc r2
    mov a,r2
    jnz slc_2
    inc r3
slc_2:
    djnz r4,sl_copy
    ret

; chips: sprite 1 down the rows (fixed.inc row_write): a chip beside each
; seat with a bet in this street (rounds 1-4) -- the server moves the bets
; into the pot as the next is dealt. Rows 0-6: the hand's bottom row has
; none. Clobbers A, r0-r7.
chips:
    mov r7,#0                   ; the row
    mov r4,#(ROWS+ROW_SPR)&0FFh ; its chip's Y in ROWS
    mov r5,#Y_ROW0+CHIPDY       ; and the chip's place
cp_l:
    mov a,r7                    ; its seat: us on row 6, else OPPOFF + row
    xrl a,#6
    jz cp_seat
    mov r0,#OPPOFF
    movx a,@r0
    add a,r7
cp_seat:
    mov r1,a
    call our_round              ; (the showdown's pot has the bets, but the
    mov r3,#SPR_OFF             ; seats still show them)
    jz cp_put
    mov a,r1
    cpl a
    mov r3,a
    mov r0,#NSEAT
    movx a,@r0
    add a,r3                    ; carry: NSEAT > seat
    mov r3,#SPR_OFF
    jnc cp_put
    mov a,r1
    call seat_base
    mov a,r2
    add a,#SE_BET
    mov r2,a
    clr a
    addc a,r3
    mov r3,a
    call st_byte
    mov r1,a
    inc r2                      ; (no seat's bet straddles a 256)
    call st_byte
    orl a,r1
    mov r3,#SPR_OFF
    jz cp_put
    mov a,r5
    mov r3,a
cp_put:
    mov r2,#ROWS>>8
    mov a,r4
    call poke_at
    mov a,r3
    call poke_b
    mov a,r4
    add a,#ROW_TAB
    mov r4,a
    mov a,r5
    add a,#ROW_PITCH
    mov r5,a
    inc r7
    mov a,r7
    xrl a,#7
    jnz cp_l
    ret

    if $ > 0D00h
    error "page $C has run out"
    endif

; ===========================================================================
    org 0D00h
; lr_draw: T1 = a page of lastResult from RPAGE: what fits in 16, cut at a
; space; RNEXT the next page's start (0: the first again). Clobbers A,
; r0-r7.
lr_draw:
    call lb_clear
    mov r0,#RPAGE
    movx a,@r0
    mov r2,a                    ; the byte looked at
    mov r3,#0
    mov r5,#0                   ; k: how many so far
    mov r7,#0                   ; the last space's k
lr_l:
    mov a,r2                    ; past the field: its end
    add a,#256-80
    jc lr_nul
    call st_byte
    jz lr_nul
    mov r4,a
    mov a,r5
    xrl a,#16
    jz lr_17
    mov a,r4
    xrl a,#' '
    jnz lr_n
    mov a,r5
    mov r7,a
lr_n:
    inc r5
    inc r2
    jmp lr_l
lr_nul:                         ; the end: this page is the last
    mov r4,#0
    jmp lr_put
lr_17:                          ; a 17th (r2 at it): cut at the last space
    mov a,r4
    xrl a,#' '
    jz lr_sp
    mov a,r7
    jz lr_long
    mov r5,a                    ; r7 of them; the next after the space
    mov a,r2
    add a,#256-16
    add a,r5
    inc a
    mov r4,a
    jmp lr_put
lr_sp:                          ; 16, the next after the space
    mov a,r2
    inc a
    mov r4,a
    jmp lr_put
lr_long:                        ; no space: 16, the next from the 17th
    mov a,r2
    mov r4,a
lr_put:
    mov r0,#RNEXT
    mov a,r4
    movx @r0,a
    mov r0,#RPAGE               ; r5 characters from the start, centred
    movx a,@r0
    mov r2,a
    mov a,r5
    cpl a
    add a,#17
    clr c
    rrc a
    add a,#LINEBUF
    mov r1,a
lr_c:
    mov a,r5
    jz lr_say
    call st_byte
    movx @r1,a
    inc r1
    inc r2
    dec r5
    jmp lr_c
lr_say:
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    movx @r0,a
    mov r4,#L_BOT
    jmp say_line_c

; stack: a stack of chips for r5:r4 (kept) into the sprite shape at r0: one
; from 1, two from 25, three from 100, four from 250, from the bottom, a
; line of felt between. Clobbers A, r0, r1, r6.
stack:
    mov r1,#STACK_N
    mov a,r5
    jnz sk_n
    mov r1,#0
    mov a,r4
    jz sk_n
    inc r1
    add a,#256-25
    jnc sk_n
    inc r1
    mov a,r4
    add a,#256-100
    jnc sk_n
    inc r1
    mov a,r4
    add a,#256-250
    jnc sk_n
    inc r1
sk_n:
    mov a,r1
    cpl a
    add a,#STACK_N+1
    mov r6,a                    ; the empty places above
    mov r1,#STACK_N
    mov a,#P1_VDC
    outl p1,a
sk_l:
    clr a                       ; felt
    movx @r0,a
    inc r0
    mov a,r6
    jz sk_chip
    dec r6
    clr a
    jmp sk_put
sk_chip:
    mov a,#7Eh                  ; a chip, edge on
sk_put:
    movx @r0,a
    inc r0
    djnz r1,sk_l
    mov a,#P1_MAIN
    outl p1,a
    ret

; t0_draw: T0 = "POT n" on the left, our purse "$n" on the right, each
; with its stack of chips to its left. Clobbers A, r0-r7.
t0_draw:
    call lb_clear
    mov a,#s_pot&0FFh
    call lb_str
    mov r3,#0
    mov r2,#GAME_POT
    call st_byte
    mov r4,a
    inc r2
    call st_byte
    mov r5,a
    mov r0,#vdc_spr2_shape
    call stack
    mov r1,#LINEBUF+4
    call put_u16
    clr a                       ; our purse
    call seat_base
    mov a,r2
    add a,#SE_PURSE
    mov r2,a
    clr a
    addc a,r3
    mov r3,a
    call st_byte
    mov r4,a
    inc r2
    call st_byte
    mov r5,a
    mov r0,#vdc_spr3_shape
    call stack
    mov r1,#LINEBUF+11
    mov a,#'$'
    movx @r1,a
    inc r1
    call put_u16
    mov r4,#L_TOP
    jmp say_line

    if $ > 0E00h
    error "page $D has run out"
    endif

    PAGE_TAIL
