; p2.inc -- page 2: the game's main loop. Each frame, in the quiet stretch
; after the last band: the keyboard and the stick, a step of the request in
; flight (page 1), and the table's pages -- page 3 when a state has come,
; page 4 every frame. All by farcall from here, the top level
; (tools/checkstack.py).

POLLN   equ 30                  ; frames between polls: half a second

    PAGE_HEAD
    db e_main&0FFh              ; entry 0: from the boot, one way
e_main:
    jmp main

main:
    mov r0,#LROUND              ; nothing shown yet
    mov a,#0FFh
    movx @r0,a
    mov r0,#LACTV
    movx @r0,a
    clr a
    mov r0,#POLLW
    movx @r0,a
    mov r0,#ACT
    movx @r0,a
    mov r0,#HELP
    movx @r0,a
    mov r0,#SPOT
    movx @r0,a
    mov r0,#MSEL
    movx @r0,a
    mov r0,#NMOV
    movx @r0,a
    mov r0,#RPAGE
    movx @r0,a
    mov r0,#RPCLK
    movx @r0,a
    mov r0,#OPPOFF
    inc a
    movx @r0,a
ml:
    call frame_sync
    call key_poll
    call joy_poll
    mov r0,#HELP                ; the help screen up: a key takes it down
    movx a,@r0
    jz ml_keys
    call help_end
    jmp ml_net
ml_keys:
    mov r0,#KEY                 ; H or ?: the help screen
    movx a,@r0
    xrl a,#K_H
    jz ml_help
    xrl a,#K_H!K_QUEST
    jz ml_help
    xrl a,#K_QUEST!K_L          ; L: leave the table
    jnz ml_net
    mov r0,#ACT
    mov a,#ACT_LEAVE
    movx @r0,a
    jmp ml_net
ml_help:
    mov r0,#HELP
    mov a,#1
    movx @r0,a
    mov r5,#HC_TABLE
    mov r7,#PG_HELP
    mov a,#E_HELP
    FCALL farcall
ml_net:
    mov r7,#PG_NET
    mov a,#E_NSTEP
    FCALL farcall
    mov r0,#NSTATE
    movx a,@r0
    jnz ml_tick                 ; a request is out
    mov r0,#ACTSENT             ; we have left: the table list
    movx a,@r0
    xrl a,#ACT_LEAVE
    jnz ml_have
    mov r7,#E_TABLES
    mov a,#PG_BOOT
    jmp far_go
ml_have:
    mov r0,#NETHAVE
    movx a,@r0
    xrl a,#NH_STATE
    jnz ml_idle
    clr a                       ; a state: taken
    movx @r0,a
    mov r0,#HELP                ; (not while the help is up)
    movx a,@r0
    jnz ml_idle
    mov r7,#PG_STATE
    mov a,#E_STATE
    FCALL farcall
    mov r7,#PG_STATE2
    mov a,#E_STATE
    FCALL farcall
ml_idle:
    mov r0,#ACT                 ; an action goes at once; a poll waits its turn
    movx a,@r0
    jnz ml_begin
    mov r0,#POLLW
    movx a,@r0
    jz ml_begin
    dec a
    movx @r0,a
    jmp ml_tick
ml_begin:
    mov r0,#POLLW
    mov a,#POLLN
    movx @r0,a
    mov r7,#PG_NET
    mov a,#E_NBEGIN
    FCALL farcall
ml_tick:
    mov r0,#HELP                ; the table's ticks wait for its screen
    movx a,@r0
    jnz ml_snd
    mov r7,#PG_TICK
    mov a,#E_TICK
    FCALL farcall
ml_snd:
    call snd_go
    jmp ml

; help_end: a key or the button takes the help screen down: the table comes
; back afresh with the next state, polled at once. The key is spent.
; Clobbers A, r0.
help_end:
    mov r0,#KEY
    movx a,@r0
    xrl a,#K_NONE
    jnz he_go
    mov r0,#JOYEV
    movx a,@r0
    anl a,#J_FIRE
    jnz he_go
    ret
he_go:
    mov r0,#KEY
    mov a,#K_NONE
    movx @r0,a
    clr a
    mov r0,#JOYEV
    movx @r0,a
    mov r0,#POLLW
    movx @r0,a
    mov r0,#HELP
    movx @r0,a
    mov r0,#LROUND              ; the table afresh
    mov a,#0FFh
    movx @r0,a
    ret

    if $ > 900h
    error "page $8 has run out"
    endif

; ===========================================================================
    org 900h
    include "input.inc"

; ===========================================================================
    org 0A00h
    include "snd.inc"

    PAGE_TAIL
