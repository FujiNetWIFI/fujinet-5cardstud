; fixed.inc -- the fixed half, $400-$7FF: the display engine (Battleship's,
; which was M.U.L.E.'s, reshaped for a card table).
;
; The 8244's 28 characters a frame are re-pointed between lines of text and
; rows of cards by a line-counter interrupt, and the 8048 holds A11 at 0 for
; a whole interrupt routine -- so everything here runs below $800, and every
; table an interrupt reads with MOVP sits in the page of the code that reads
; it. The tables are cartridge RAM: the main program writes them through the
; mailbox's POKE register (disp.inc) and never touches the VDC's object
; registers itself (the sprites' shapes aside).
;
; Every screen is the same: text line T0 at the top (the four quads, written
; in the vblank with the sprites), eight rows of eleven singles, and T1 at
; the bottom. On the table the rows are cards on the felt (the grid in fill
; mode makes the cards' faces); on a text screen they are lines of text on
; black, the singles side by side (MODE_TEXT) and the grid empty. One band
; list (page $7, screen.inc), one counter interrupt per band:
;
;   h_top   line 1: row 0 (staged by the frame before's T1 band), then the
;           grid's cells -- hidden until T0 has ended, and its registers can
;           only be written while it is
;   h_grid  T0's end: the cards on
;   h_row   rows 1-6: a row's singles out of ROWBUF, with the cards switched
;           off for a few lines in the middle (SEPA-SEPB) -- the gap between
;           two rows of cards, which has to straddle the grid's row boundary
;           -- then the next row staged into ROWBUF
;   h_row2  row 7, the hand's bottom: the same with the cards left on, so the
;           hand's cards are two grid rows tall
;   h_txt   T1, then row 0 staged for the next frame
;
; A band re-arms the counter first (tim_isr), then does its VDC writes, and
; only then fetches its argument (band_arg). A row's argument is the next
; row's table (row_copy), and the rows are staged in order, so its Y is the
; last one's plus a row.
;
; Assembled into every page's build; build.sh checks they all agree.

; ISR_IN / ISR_VDC / ISR_OUT: the frame round every band. Register bank 1;
; P1 saved in r6, A in r7.
ISR_IN  macro
    sel rb1
    mov r7,a
    endm
ISR_VDC macro
    in a,p1
    mov r6,a
    mov a,#P1_VDC
    outl p1,a
    endm
ISR_CTL macro val               ; the control register: foreground, grid
    mov r0,#vdc_control
    mov a,#val
    movx @r0,a
    endm
ISR_OUT macro
    mov a,r6
    outl p1,a
    mov a,r7
    retr
    endm

; ===========================================================================
;  page $4: vectors, boot, page switching, the row writer, h_top
; ===========================================================================
    org 400h
    jmp start                   ; $400 reset
    jmp vbl_isr                 ; $402 external IRQ: the VDC's vblank
    jmp tim_isr                 ; $404 counter overflow: a band is due
    jmp start                   ; $406 (the BIOS's own vblank chain: unused)
    jmp start                   ; $408 (after the BIOS's game select: unused)
    retr                        ; $40A
    nop

; start: a console RESET does not reset the cartridge, so the page mapped and
; every table survive it; boot puts back what it needs.
start:                          ; (a reset leaves interrupts off, the counter
    mov a,#P1_MAIN              ; stopped)
    outl p1,a
    clr a
    call sel_page               ; page 0
    mov r0,#ROWBUF              ; internal RAM $20-$3C
    mov r2,#03Dh-ROWBUF
    clr a
st_clr:
    mov @r0,a
    inc r0
    djnz r2,st_clr
st_boot:
    jmp 800h                    ; page 0's entry 0 (A = 0): the boot

; sel_page: map page A at $800-$F1F. Main program, P1 = P1_MAIN.
; Clobbers A, r0, r1.
sel_page:
    mov r1,#CURPG
    mov @r1,a
    mov r0,#FN_W_REGSEL
    mov a,#FN_REG_CODEBANK
    movx @r0,a
    inc r0
    mov a,@r1
    movx @r0,a
    ret

; farcall: entry A of page r7, then the caller's page back. From a page:
;     mov r7,#PG_x / mov a,#E_y / FCALL farcall
; One level: an entry it runs may not farcall. Results come back in r2-r5 or
; external RAM (A and r0, r1, r6 are spent getting home). Clobbers r6.
farcall:
    mov r6,a
    mov r1,#CURPG
    mov a,@r1
    mov r1,#OLDPG
    movx @r1,a
    mov a,r7
    call sel_page
    mov a,r6
    call 800h                   ; the page's dispatcher (page.inc)
    sel mb0
    mov r1,#OLDPG
    movx a,@r1
    call sel_page
    ret

; row_write: a row out of ROWBUF into the singles -- Y (r4), then each one's
; (pointer, control); their X is the screen's (obj_setup). The control
; register gets r3 at SEPA and r2 at SEPB: a row band's gap between two rows
; of cards. Each is placed after one register of one single (SEPA = single,
; SEPAF = 0 its Y, 1 its pointer, 2 its control), so that it lands in the
; horizontal blank: mid-line, the line it splits jitters by an interrupt's
; latency. VDC selected, foreground off. NCOLS x 15 cycles, and 14 for the
; two control writes. Clobbers A, r0, r1.
SEPW    macro k,f,nxt
    if (k=SEPA)&&(f=SEPAF)
    mov r0,#vdc_control
    mov a,r3
    movx @r0,a
    mov r0,#vdc_char0+4*k+nxt
    endif
    if (k=SEPB)&&(f=SEPBF)
    mov r0,#vdc_control
    mov a,r2
    movx @r0,a
    mov r0,#vdc_char0+4*k+nxt
    endif
    endm
row_write:
    mov r1,#ROWBUF
    mov r0,#vdc_char0
K   set 0
    rept NCOLS
    mov a,r4
    movx @r0,a                  ; Y
    inc r0
    inc r0                      ; (X)
    SEPW K,0,2
    mov a,@r1
    movx @r0,a                  ; pointer
    inc r0
    inc r1
    SEPW K,1,3
    mov a,@r1
    movx @r0,a                  ; control
    inc r0
    inc r1
    SEPW K,2,4
K   set K+1
    endm
    ret

; h_top_b: row 0 (staged by band_out), the grid's cells, row 1 staged.
; Line 1: T0 is due at Y_T0, the cards at h_grid (h_top).
h_top_b:
    ISR_CTL CTL_OFF
    mov r4,#Y_ROW0
    mov r3,#CTL_OFF
    mov r2,#CTL_OFF
    call row_write
    ISR_CTL CTL_TOP
    call grid_write
    call band_arg
    call row_copy
    ISR_OUT

    if $ > 500h
    error "page $4 has left the page"
    endif

; ===========================================================================
;  page $5: the text writer and lines T0, T1; a new screen's objects
; ===========================================================================
    org 500h

; txt_write: the 16-character line whose table is at r1 in this page, into
; the four quads interleaved: A takes columns 0, 2, 4, 6, B 1, 3, 5, 7, C and
; D the next eight (disp.inc txorder). Their X is the screen's (obj_setup).
; Jumped to, not called, to keep the interrupts one call deep: it leaves to
; vb_t0 when F0 is set (the vblank's T0), else through band_out. VDC
; selected, foreground off. About 300 cycles. Clobbers A, r0, r1, r4.
txt_write:
    mov a,r1
    movp a,@a
    mov r4,a                    ; Y
    inc r1
    mov r0,#vdc_quad0
    call tq
    mov r0,#vdc_quad1
    call tq
    mov r0,#vdc_quad2
    call tq
    mov r0,#vdc_quad3
    call tq
    jf0 tw_vbl
    jmp band_out
tw_vbl:
    jmp vb_t0
tq:                             ; one quad: Y, then its four (pointer, control)
    mov a,r4
    movx @r0,a
    inc r0
    inc r0                      ; (X)
    rept 4
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r1
    inc r0
    inc r0
    inc r0
    endm
    ret

; band_out: where the text writer leaves a band: the foreground on, the
; band list on, and row 0 staged for the next frame's h_top.
band_out:
    ISR_CTL CTL_TBL             ; (a text screen's grid is empty)
    call band_arg
    mov a,#ROWS&0FFh
    call row_copy
    ISR_OUT

; obj_setup: a new screen's objects, A = MODE: the singles' X (the table's
; places, or side by side on a text screen: MODE_TEXT) and Y parked below
; the screen, the quads' X, the grid's columns empty (h_top fills them).
; VDC selected, the grid and the foreground off. Clobbers A, r0-r2.
obj_setup:
    mov r1,#oxtab&0FFh
    jb2 os_text
    jmp os_go
os_text:
    mov r1,#oxtxt&0FFh
os_go:
    mov r0,#vdc_char0
    mov r2,#12
os_l:
    mov a,#0F8h
    movx @r0,a
    inc r0
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r0
    inc r0
    inc r1
    djnz r2,os_l
    mov r0,#vdc_quad0+1         ; the quads' X
    mov a,#X_TEXT
    movx @r0,a
    mov r0,#vdc_quad1+1
    mov a,#X_TEXT+8
    movx @r0,a
    mov r0,#vdc_quad2+1
    mov a,#X_TEXT+64
    movx @r0,a
    mov r0,#vdc_quad3+1
    mov a,#X_TEXT+72
    movx @r0,a
    mov r0,#vdc_gridv0
    mov r2,#10
    clr a
os_g:
    movx @r0,a
    inc r0
    djnz r2,os_g
    ret
oxtab:                          ; the table's X: the name, then each card's
K   set 0                       ; rank and suit, a card every other column
    rept NAMEW
    db NAMEX+8*K
K   set K+1
    endm
K   set 0
    rept NCARDS
    db 8+16*(CARD_COL0+2*K)+CARDX, 16+16*(CARD_COL0+2*K)+CARDX
K   set K+1
    endm
    rept 12-NCOLS
    db 0
    endm
oxtxt:                          ; a text screen's: side by side, centred
K   set 0
    rept 12
    db X_ROWTXT+8*K
K   set K+1
    endm

T0: db TXT_TAB dup (0)          ; the text lines: TXT_TAB bytes each
T1: db TXT_TAB dup (0)
    if $ > 600h
    error "page $5 has left the page"
    endif

; ===========================================================================
;  page $6: the rows and the grid
; ===========================================================================
    org 600h

; row_copy: the row table at offset A of this page into ROWBUF; its Y is r4's
; plus a row (the rows are staged in order, row 0's Y put by h_top). About
; 150 cycles. Clobbers A, r0-r2.
row_copy:
    mov r1,a
    mov a,r4
    add a,#ROW_PITCH
    mov r4,a
    mov r0,#ROWBUF
    mov r2,#ROW_TAB
rc_l:
    mov a,r1
    movp a,@a
    mov @r0,a
    inc r0
    inc r1
    djnz r2,rc_l
    ret

; grid_write: the grid's ten columns from GRIDTAB (bit r: a card in row r).
; VDC selected, the grid disabled (its registers are closed while it shows).
; Clobbers A, r0-r2.
grid_write:
    mov r1,#GRIDTAB&0FFh
    mov r0,#vdc_gridv0
    mov r2,#10
gw_l:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    djnz r2,gw_l
    ret

; h_grid_b: T0 has ended: the cards on (h_grid). GRIDNOP pads the write
; into the horizontal blank: mid-line it would split a line, jittering.
h_grid_b:
    rept GRIDNOP
    nop
    endm
    ISR_CTL CTL_TBL
    call band_arg
    ISR_OUT

; snd_vbl: the vblank's sound, A = SNDCNT (nonzero), r1 at it. A queued one
; waits for the main loop; a playing one counts down and stops at 0. VDC
; selected. Clobbers A, r0.
snd_vbl:
    jb7 sv_x
    dec a
    mov @r1,a
    jnz sv_x
    mov r0,#vdc_soundctrl
    movx @r0,a
sv_x:
    ret

GRIDTAB:                        ; 10 columns: bit r = a card in row r
    db 10 dup (0)
ROWS:                           ; NROWS x ROW_TAB, row 0 first
    db NROWS*ROW_TAB dup (0)
    if $ > 700h
    error "page $6 has left the page"
    endif

; ===========================================================================
;  page $7: the bands and both interrupts; the sprites
; ===========================================================================
    org 700h

; The band list, poked per screen (screen.inc). BL_FIRST is 256 minus the
; line of the first band; each band is (handler slot, argument, 256 - lines
; to the next band, or 0 for the last).
SCRCOL  equ 700h                ; the colour register for this screen
BL_FIRST equ 701h
BANDS   equ 702h
NBANDS  equ 10
    db 0, 0
    db NBANDS*3 dup (0)

; vbl_isr: once a frame, in the blank. Arms the first band -- exact anywhere
; in the blank, because the counter only counts display lines (T1 is
; vblank|hblank) -- then the sprites and T0 (and vb_t0). The interrupts nest
; one call deep, so the main program may nest six.
vbl_isr:
    ISR_IN
    ISR_VDC
    mov r0,#vdc_status
    movx a,@r0                  ; reading the status drops the IRQ
    mov a,#BL_FIRST&0FFh
    movp a,@a
    mov t,a
    mov r5,#BANDS&0FFh
    mov r1,#SNDCNT              ; the sound's frame
    mov a,@r1
    jz vb_q
    call snd_vbl
vb_q:
    mov r1,#FRAMES
    inc @r1
    ISR_CTL CTL_OFF
    mov a,#SCRCOL&0FFh
    movp a,@a
    mov r0,#vdc_color
    movx @r0,a
    call spr_write
    mov r1,#T0&0FFh
    clr f0                      ; (RETR gives the main program its F0 back)
    cpl f0
    jmp txt_write

; vb_t0: the vblank after T0: a new screen's objects (MODE_SETUP), and the
; first frame's row 0; then the foreground on (the table's grid waits for
; h_grid).
vb_t0:
    mov r1,#MODE
    mov a,@r1
    jb1 vb_setup
vb_on:
    ISR_CTL CTL_TOP
    ISR_OUT
vb_setup:
    anl a,#~MODE_SETUP
    mov @r1,a
    call obj_setup
    mov a,#ROWS&0FFh
    call row_copy
    jmp vb_on

; tim_isr: a band is due. The counter is re-armed first, plus what it has
; counted since the overflow: a band that starts late (a long vblank)
; doesn't make the next one late. First, because it's a read-add-write of
; T, and done a whole row write later it can straddle the next line's
; count and lose it -- every frame, in every band, the rows walking down
; the screen a line a band. The last band ticks QUIET: the frame's stretch
; with nothing due (input.inc frame_sync). Then the handler; its argument
; comes after its writes (band_arg).
tim_isr:
    ISR_IN
    mov a,r5
    add a,#2
    movp a,@a
    mov r3,a
    mov a,t
    add a,r3
    mov t,a
    mov a,r3
    jnz ti_go
    mov r1,#QUIET
    inc @r1
ti_go:
    ISR_VDC
    mov a,r5
    movp a,@a
    jmpp @a                     ; the handler slot -> the handler
jtab:
J_TOP   equ $&0FFh
    db h_top&0FFh
J_GRID  equ $&0FFh
    db h_grid&0FFh
J_ROW   equ $&0FFh
    db h_row&0FFh
J_ROW2  equ $&0FFh
    db h_row2&0FFh
J_TXT   equ $&0FFh
    db h_txt&0FFh

h_top:
    jmp h_top_b
h_grid:
    jmp h_grid_b

; h_row: a row of cards, the next one staged (argument: its table, 0FFh
; none). h_row2: the hand's bottom row, the cards on throughout.
h_row:
    ISR_CTL CTL_GON
    mov r3,#CTL_OFF
    jmp hr_w
h_row2:
    ISR_CTL CTL_GON
    mov r3,#CTL_GON
hr_w:
    mov r2,#CTL_GON
    call row_write
    if PADN>0
    mov r2,#PADN
    djnz r2,$
    endif
    ISR_CTL CTL_TBL
    call band_arg
    inc a
    jz hr_done
    dec a
    call row_copy
hr_done:
    ISR_OUT

; h_txt: the line whose table is at offset (argument) of page $5 (band_out
; moves r5 on after the write).
h_txt:
    ISR_CTL CTL_GON             ; (the table's T1 is beside the hand's cards)
    mov a,r5
    inc a
    movp a,@a
    mov r1,a
    clr f0
    jmp txt_write

; band_arg: A = this band's argument, and r5 on to the next band. Clobbers
; r3.
band_arg:
    mov a,r5
    inc a
    movp a,@a
    mov r3,a
    mov a,r5
    add a,#3
    mov r5,a
    mov a,r3
    ret

; spr_write: the four sprites' (Y, X, attribute) from SPRTAB. Their shapes
; the main program writes itself: the VDC takes those with the display on.
; VDC selected, foreground off. Clobbers A, r0-r2.
spr_write:
    mov r1,#SPRTAB&0FFh
    mov r0,#vdc_spr0_ctrl
    mov r2,#4
sw_l:
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r1
    mov a,r1
    movp a,@a
    movx @r0,a
    inc r0
    inc r0
    inc r1
    djnz r2,sw_l
    ret

SPRTAB:                         ; 4 x (Y, X, attribute)
    db 12 dup (0)

    if $ > 800h
    error "page $7 has left the page"
    endif
