; p5.inc -- page 5: the help screen, on H or ?. A text screen of the keys
; for where we are (r5: HC_*); the caller takes it down again on any key or
; the button (page 2 at the table, page 0 for the table list). And the
; boot's name entry, another text screen.

    PAGE_HEAD
    db e_help&0FFh, e_name&0FFh
e_help:
    jmp help_show
e_name:
    jmp name_entry

; help_show: the help for context r5: its title in T0, its rows, a line in
; T1. Clobbers A, r0-r7.
help_show:
    mov r0,#TMP1
    mov a,r5
    movx @r0,a
    call scr_text
    mov r0,#TMP1                ; its table: 10 strings, T0 first
    movx a,@r0
    mov r6,a
    rl a
    rl a
    add a,r6
    rl a                        ; 10 x the context
    add a,#helptab&0FFh
    mov r0,#TMP0
    movx @r0,a
    movp a,@a
    mov r4,#L_TOP
    call hl_say
    clr a                       ; the rows
hs_l:
    mov r0,#TMP2
    movx @r0,a
    mov r0,#TMP0
    movx a,@r0
    inc a
    movx @r0,a
    movp a,@a
    jz hs_n
    mov r6,a
    mov r0,#TMP2
    movx a,@r0
    mov r4,a
    mov a,r6
    call hl_row
hs_n:
    mov r0,#TMP2
    movx a,@r0
    inc a
    mov r6,a
    xrl a,#NROWS
    jz hs_t1
    mov a,r6
    jmp hs_l
hs_t1:
    mov r0,#TMP0
    movx a,@r0
    inc a
    movp a,@a
    mov r4,#L_BOT
    jmp hl_say

; Each context: T0, rows 0-7 (0: none; row 0 is never used, it would
; touch T0), T1.
helptab:
    db h_t0&0FFh                                        ; HC_TABLES
    db 0, h_stick&0FFh, h_tpick&0FFh, 0, h_btn&0FFh, h_sit&0FFh, 0, 0
    db h_back&0FFh
    db h_t1&0FFh                                        ; HC_TABLE
    db 0, h_lr&0FFh, h_amove&0FFh, h_btnp&0FFh, h_f&0FFh, h_c&0FFh
    db h_b&0FFh, h_r&0FFh
    db h_leave&0FFh

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "disp.inc"

; ===========================================================================
    org 0A00h
    include "text.inc"

s_yname: db "  YOUR NAME?",0
s_nk1:   db "TYPE IT THEN ENT",0
s_nk2:   db "CLR ERASES",0

    if $ > 0B00h
    error "page $A has run out"
    endif

; ===========================================================================
    org 0B00h
TEXTSCR equ 1
    include "screen.inc"

    if $ > 0C00h
    error "page $B has run out"
    endif

; ===========================================================================
    org 0C00h
    include "input.inc"

    if $ > 0D00h
    error "page $C has run out"
    endif

; ===========================================================================
    org 0E00h

; hl_say: T-line r4 = string A of this page. hl_row: row r4, white.
; Clobbers A, r0-r7.
hl_say:
    call hl_lb
    jmp say_line
hl_row:
    call hl_lb
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    movx @r0,a
    jmp row_line
hl_lb:                          ; LINEBUF = string A of this page
    mov r6,a
    call lb_clear
    mov r0,#LINEBUF
    mov a,r6
    mov r1,a
hll_l:
    mov a,r1
    movp a,@a
    jz hll_x
    movx @r0,a
    inc r0
    inc r1
    jmp hll_l
hll_x:
    ret

h_t0:    db "HELP: TABLES",0
h_stick: db "STICK U/D",0
h_tpick: db " A TABLE",0
h_btn:   db "BUTTON",0
h_sit:   db " SIT DOWN",0
h_back:  db "ANY KEY: BACK",0
h_t1:    db "HELP: THE TABLE",0
h_lr:    db "STICK L/R",0
h_amove: db " A MOVE",0
h_btnp:  db "BUTTON PLAY",0
h_f:     db "F FOLD",0
h_c:     db "C CALL/CHK",0
h_b:     db "B BET",0
h_r:     db "R RAISE",0
h_leave: db "L LEAVE U/D SEAT",0

    if $ > 0F00h
    error "page $E has run out"
    endif

; ===========================================================================
    org 0D00h

; name_entry: SCNAME from the keyboard: letters, digits and spaces (not
; first), at most 8; CLR rubs out, ENT ends. Clobbers A, r0-r7.
name_entry:
    call scr_text
    mov r4,#L_TOP
    mov a,#s_yname&0FFh
    call say
    mov r4,#L_BOT
    mov a,#s_nk1&0FFh
    call say
    mov r4,#6
    mov a,#s_nk2&0FFh
    call say_row
    clr a
    mov r0,#SCNAME
    movx @r0,a
    mov r0,#NAMEN
    movx @r0,a
ne_l:
    call frame_sync
    call key_poll
    mov r0,#KEY
    movx a,@r0
    mov r5,a
    xrl a,#K_NONE
    jz ne_draw
    mov a,r5
    xrl a,#K_ENT
    jz ne_end
    xrl a,#K_ENT!K_CLR
    jz ne_rub
    mov a,r5                    ; a character: its ASCII
    add a,#256-48
    jc ne_draw
    mov a,r5
    add a,#keychar&0FFh
    movp a,@a
    call name_ch
    jz ne_draw
    mov r5,a
    mov r0,#NAMEN
    movx a,@r0
    xrl a,#8
    jz ne_draw                  ; full
    xrl a,#8
    jnz ne_add
    mov a,r5                    ; no leading space
    xrl a,#' '
    jz ne_draw
    mov r0,#NAMEN
    movx a,@r0
ne_add:
    mov r1,a
    inc a
    movx @r0,a
    mov a,r1
    add a,#SCNAME
    mov r0,a
    mov a,r5
    movx @r0,a
    inc r0
    clr a
    movx @r0,a
    jmp ne_draw
ne_rub:
    mov r0,#NAMEN
    movx a,@r0
    jz ne_draw
    dec a
    movx @r0,a
    add a,#SCNAME
    mov r0,a
    clr a
    movx @r0,a
    jmp ne_draw
ne_end:
    mov r0,#NAMEN
    movx a,@r0
    jz ne_draw
    ret
ne_draw:
    call lb_clear               ; the name, then '_' blinking after it
    mov r1,#0
nd_l:
    mov a,r1
    add a,#SCNAME
    mov r0,a
    movx a,@r0
    jz nd_x
    mov r6,a
    mov a,r1
    add a,#LINEBUF+1
    mov r0,a
    mov a,r6
    movx @r0,a
    inc r1
    jmp nd_l
nd_x:
    mov a,r1
    add a,#LINEBUF+1
    mov r1,a
    mov r0,#FRAMES
    mov a,@r0
    anl a,#16
    jz nd_put
    mov a,#'_'
    movx @r1,a
nd_put:
    mov r0,#TXCOL
    mov a,#C_WHITE
    movx @r0,a
    inc r0
    movx @r0,a
    mov r4,#3
    call row_line
    jmp ne_l

; name_ch: A as a name's character -- a letter upper-cased, a digit or a
; space as it is; 0 for anything else. Clobbers r6.
name_ch:
    mov r6,a
    xrl a,#' '
    jz nc_ok
    mov a,r6
    add a,#256-'a'              ; 'a'..'z': up
    jnc nc_1
    add a,#256-26
    jc nc_1
    mov a,r6
    add a,#256-32
    ret
nc_1:
    mov a,r6                    ; 'A'..'Z'
    add a,#256-'A'
    jnc nc_2
    add a,#256-26
    jnc nc_ok
nc_2:
    mov a,r6                    ; '0'..'9'
    add a,#256-'0'
    jnc nc_no
    add a,#256-10
    jnc nc_ok
nc_no:
    clr a
    ret
nc_ok:
    mov a,r6
    ret

; A key's character (readkey codes 0-47; 0: none).
keychar:
    db "0123456789",0,0," ?LP+WERTUIOQSDFGHJKAZXCVBM.-*/=YN",0,0


    if $ > 0E00h
    error "page $D has run out"
    endif

    PAGE_TAIL
