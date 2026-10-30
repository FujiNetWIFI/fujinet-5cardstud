; p3.inc -- page 3: the table, when a state has come. Everything the ticks
; need is taken here (the reply window is repainted once the next request
; is under way): the round, who is to move and their clock, and on our turn
; our moves. Then the rows whose seat changed (a signature a row: a whole
; table is eight rows and frames of work). Page 6 does the rest of the
; state, in the same pass of the main loop: T0, T1, the sounds.
;
; Rows 0-5 are the opponents, seat OPPOFF on: seat 1 normally, 2 when there
; are seven and the spotlight is on the last. Rows 6-7 are us, seat 0 (the
; server puts us first).

    PAGE_HEAD
    db e_state&0FFh, e_title&0FFh
e_state:
    jmp on_state
e_title:
    jmp title

on_state:
    call hdr_copy
    mov r0,#LROUND              ; the table's screen, afresh
    movx a,@r0
    inc a
    jnz os_up
    call scr_table
    call rsig_clear
os_up:
    call spot_mark              ; the spotlight's marker leads the rows
    jmp rows_draw

; hdr_copy: the header's fields into XRAM; the clock's second begun; on our
; turn our moves (MOVES, MAMT, NMOV), and at its start the move picked
; (MSEL: the first that isn't a fold); and the spotlight on whoever is to
; move, when that changes. Clobbers A, r0-r7.
hdr_copy:
    mov r3,#0
    mov r5,#hdr_tab&0FFh
hc_l:
    mov a,r5
    movp a,@a                   ; the field's offset (0: done)
    jz hc_moves
    mov r2,a
    call st_byte
    mov r4,a
    inc r5
    mov a,r5
    movp a,@a                   ; where it goes
    mov r0,a
    mov a,r4
    movx @r0,a
    inc r5
    jmp hc_l
hc_moves:
    mov r0,#SECF
    mov a,#60
    movx @r0,a
    mov r0,#ACTV                ; the spotlight follows the turn
    movx a,@r0
    mov r4,a
    mov r0,#LACTV
    movx a,@r0
    xrl a,r4
    jz hc_spot
    mov a,r4
    add a,#256-MAXSEATS
    jc hc_spot
    mov a,r4
    mov r0,#SPOT
    movx @r0,a
    call opp_scroll
hc_spot:
    clr a                       ; our moves: none, unless it's our turn
    mov r0,#NMOV
    movx @r0,a
    mov a,r4
    jnz hc_x
    call our_round
    jz hc_x
    mov r2,#GAME_NMOVES
    call st_byte
    mov r7,a                    ; at most 5
    add a,#256-NMOVES-1
    jnc hc_n
    mov r7,#NMOVES
hc_n:
    mov a,r7
    jz hc_x
    mov r0,#NMOV
    movx @r0,a
    mov r5,#0                   ; move r5: code, amount
hc_mv:
    call move_copy
    inc r5
    mov a,r5
    xrl a,r7
    jnz hc_mv
    mov r0,#LACTV               ; our turn has just begun: a move picked
    movx a,@r0
    jz hc_keep
    mov r0,#MSEL
    clr a
    movx @r0,a
    mov r0,#MOVES               ; the first that isn't a fold
    movx a,@r0
    xrl a,#'f'
    jnz hc_x
    mov a,r7
    dec a
    jz hc_x
    mov r0,#MSEL
    mov a,#1
    movx @r0,a
    ret
hc_keep:
    mov r0,#MSEL                ; still ours: the same one, if it's still there
    movx a,@r0
    cpl a
    add a,r7                    ; carry: NMOV > MSEL
    jc hc_x
    clr a
    movx @r0,a
hc_x:
    ret

hdr_tab:                        ; (offset, where)
    db GAME_ROUND, ROUND
    db GAME_ACTIVE, ACTV
    db GAME_MOVETIME, MOVET
    db GAME_COUNT, NSEAT
    db 0

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

; move_copy: move r5 of the reply into MOVES (its code) and MAMT (the number
; in its name: "bet 5", "raise 10"; 0 for none). Clobbers A, r0-r4, r6.
move_copy:
    mov a,r5                    ; 88 + 13 r5
    rl a
    rl a
    mov r2,a
    rl a
    add a,r2                    ; 12 r5
    add a,r5
    add a,#GAME_MOVES
    mov r2,a
    mov a,r5
    rl a
    add a,#MOVES
    mov r1,a
    call st_byte                ; the code's two letters
    movx @r1,a
    inc r1
    inc r2
    call st_byte
    movx @r1,a
    inc r2                      ; the name: its digits
    inc r2
    mov r4,#0
    mov r1,#9
mc_l:
    call st_byte
    jz mc_x
    add a,#256-'0'
    jnc mc_n
    mov r6,a
    add a,#256-10
    jc mc_n
    mov a,r4                    ; r4 = 10 r4 + digit
    rl a
    mov r4,a
    rl a
    rl a
    add a,r4
    add a,r6
    mov r4,a
mc_n:
    inc r2
    djnz r1,mc_l
mc_x:
    mov a,r5
    add a,#MAMT
    mov r0,a
    mov a,r4
    movx @r0,a
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

s_title: db "  5 CARD STUD",0
s_fuji:  db "FUJINET ODYSSEY2",0


; ===========================================================================
    org 0B00h
    include "screen.inc"
    include "stbyte.inc"

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

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
    include "table.inc"

; rows_draw: every row whose seat has changed since it was drawn; ROWSNEW
; if any has. Clobbers A, r0-r7.
rows_draw:
    clr a
    mov r0,#ROWSNEW
    movx @r0,a
rd_l:
    mov r0,#ROWN
    movx @r0,a
    call row_seat               ; CARDS, ROWNAME, ROWNC; A = the signature
    mov r4,a
    mov r0,#ROWN
    movx a,@r0
    add a,#RSIG
    mov r0,a
    movx a,@r0
    xrl a,r4
    jz rd_n
    mov a,r4
    movx @r0,a
    mov r0,#ROWSNEW
    mov a,#1
    movx @r0,a
    call grid_add               ; its new cards' faces first, then the
    call grid_flush             ; characters on them, then the faces of
    call row_put                ; any gone: neither order ever shows
    call grid_flush             ; characters on bare felt
rd_n:
    mov r0,#ROWN
    movx a,@r0
    inc a
    mov r6,a
    xrl a,#NROWS
    jz rd_g
    mov a,r6
    jmp rd_l
rd_g:
    ret

; title: the boot's title, on the table: a royal flush in our hand, a hand
; face down on the felt, the name above and below. Clobbers A, r0-r7.
title:
    call scr_table
    mov r7,#tt_flush&0FFh       ; (row, 10 card bytes) x 3
tt_l:
    mov a,r7
    movp a,@a
    inc a
    jz tt_lines
    dec a
    mov r0,#ROWN
    movx @r0,a
    inc r7
    mov r0,#CARDS
    mov r6,#2*NCARDS
tt_c:
    mov a,r7
    movp a,@a
    movx @r0,a
    inc r0
    inc r7
    djnz r6,tt_c
    mov a,#' '                  ; no name
    movx @r0,a
    mov a,r7
    mov r0,#TMP2
    movx @r0,a
    call row_put
    mov r0,#TMP2
    movx a,@r0
    mov r7,a
    jmp tt_l
tt_lines:
    call grid_flush
    mov r4,#L_TOP
    mov a,#s_title&0FFh
    call say
    mov r4,#L_BOT
    mov a,#s_fuji&0FFh
    jmp say
tt_flush:
    db 2, "??????????"
    db 6, "ahkhqhjhth"
    db 7, "ahkhqhjhth"
    db 0FFh

    if $ > 0E00h
    error "page $D has run out"
    endif

; ===========================================================================
    org 0E00h

; opp_scroll: seven opponents and the spotlight on one the rows don't show:
; OPPOFF so they do, and every row redrawn. Clobbers A, r0, r1.
opp_scroll:
    mov r0,#SPOT
    movx a,@r0
    mov r1,a
    xrl a,#7
    jz osc_2
    mov a,r1
    xrl a,#1
    jnz osc_x
    mov a,#1
    jmp osc_set
osc_2:
    mov a,#2
osc_set:
    mov r1,a
    mov r0,#OPPOFF
    movx a,@r0
    xrl a,r1
    jz osc_x
    mov a,r1
    movx @r0,a
    ; fall into rsig_clear

; rsig_clear: every row redrawn with the next state. Clobbers A, r0, r1.
rsig_clear:
    mov r0,#RSIG
    mov r1,#NROWS
    mov a,#0FFh
rs_l:
    movx @r0,a
    inc r0
    djnz r1,rs_l
osc_x:
    ret

; row_seat: row ROWN's seat into CARDS (its hand, or none: not playing or
; folded), ROWNAME (its initial) and ROWNC (tan to move, grey out of the
; hand, cyan us, else white). A = a signature of all that. Clobbers A,
; r0-r7.
row_seat:
    mov r0,#ROWN
    movx a,@r0
    mov r7,a                    ; the row
    add a,#256-6
    jnc rs_opp
    clr a                       ; 6 and 7: us
    jmp rs_seat
rs_opp:
    mov r0,#OPPOFF
    movx a,@r0
    add a,r7
rs_seat:
    mov r5,a                    ; the seat
    mov r0,#CARDS               ; none yet
    mov r1,#2*NCARDS
    clr a
rs_c:
    movx @r0,a
    inc r0
    djnz r1,rs_c
    mov a,#' '                  ; (ROWNAME follows CARDS)
    movx @r0,a
    mov a,r5                    ; a seat past the table's: an empty row
    cpl a
    mov r1,a
    mov r0,#NSEAT
    movx a,@r0
    add a,r1                    ; carry: NSEAT > seat
    jc rs_here
    clr a
    ret
rs_here:
    mov a,r5
    call seat_base
    call st_byte                ; its initial (none on the hand's bottom)
    mov r4,a
    mov a,r7
    xrl a,#7
    jnz rs_init
    mov r4,#' '
rs_init:
    mov r0,#ROWNAME
    mov a,r4
    movx @r0,a
    mov a,r2                    ; its status
    add a,#SE_STATUS
    mov r2,a
    clr a
    addc a,r3
    mov r3,a
    call st_byte
    mov r6,a
    mov r1,#C_TAN               ; to move
    mov r0,#ACTV
    movx a,@r0
    xrl a,r5
    jz rs_col
    mov r1,#C_DGREY             ; out of the hand
    mov a,r6
    xrl a,#PS_PLAYING
    jnz rs_col
    mov r1,#C_CYAN              ; us (the spotlight is a sprite's)
    mov a,r5
    jz rs_col
    mov r1,#C_WHITE
rs_col:
    mov r0,#ROWNC
    mov a,r1
    movx @r0,a
    swap a                      ; the signature so far: colour, initial
    add a,r4
    mov r4,a
    mov a,r6                    ; in the hand: its cards
    xrl a,#PS_PLAYING
    jnz rs_x
    mov a,r2                    ; its hand: 13 bytes past its status
    add a,#SE_HAND-SE_STATUS
    mov r2,a
    clr a
    addc a,r3
    mov r3,a
    mov r1,#CARDS
    mov r5,#2*NCARDS
rs_h:
    call st_byte
    movx @r1,a
    xch a,r4                    ; the signature: rotate, add
    rl a
    add a,r4
    mov r4,a
    inc r1
    inc r2
    mov a,r2
    jnz rs_h1
    inc r3
rs_h1:
    djnz r5,rs_h
rs_x:
    mov a,r4
    ret

    include "sprites.inc"

    if $ > 0F00h
    error "page $E has run out"
    endif

    PAGE_TAIL
