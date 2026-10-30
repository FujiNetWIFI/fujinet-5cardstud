; sprites.inc -- spot_mark: the spotlight's marker (sprite 0, screen.inc
; spr_table), for a state (page 3) and the stick (page 4).

; spot_mark: sprite 0's Y: beside seat SPOT's row -- us on row 6, an
; opponent on row SPOT - OPPOFF -- or below the screen if it has no row.
; Clobbers A, r0-r3.
spot_mark:
    mov r0,#SPOT
    movx a,@r0
    mov r3,a
    cpl a
    mov r1,a
    mov r0,#NSEAT
    movx a,@r0
    add a,r1                    ; carry: NSEAT > SPOT
    jnc sm_off
    mov a,r3
    jz sm_us
    mov r0,#OPPOFF
    movx a,@r0
    cpl a
    inc a
    add a,r3
    mov r3,a                    ; its row: 0-5, or none
    add a,#256-6
    jc sm_off
    mov a,r3
    jmp sm_y
sm_us:
    mov a,#6
sm_y:
    rl a                        ; Y_ROW0 + 24 x row
    rl a
    rl a
    mov r3,a
    rl a
    add a,r3
    add a,#Y_ROW0+MARKDY
    jmp sm_put
sm_off:
    mov a,#SPR_OFF
sm_put:
    mov r3,a
    mov r2,#SPRTAB>>8
    mov a,#SPRTAB&0FFh
    call poke_at
    mov a,r3
    jmp poke_b
