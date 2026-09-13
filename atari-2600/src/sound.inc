; sound.inc -- four cues, fired on edges.
;
; Two channels, and nothing here waits: a cue is set going and a frame counter
; retires it, because the display loop cannot afford to block and a
; transaction already stalls the picture for seconds at a time.
;
; AUDC picks the waveform, AUDF the divider and AUDV the volume. Channel 0
; carries everything; channel 1 is left silent so the swap stub's "or it
; screams" problem cannot arise.

SNDDEAL EQU     0               ; a card hit the table
SNDTURN EQU     1               ; it is your turn
SNDSENT EQU     2               ; your bet went out
SNDEND  EQU     3               ; the hand is over

SNDLEN  EQU     8               ; frames a cue lasts

; SNDFIRE -- start cue A.
SNDFIRE: tax
        lda     SNDCT,x
        sta     AUDC0
        lda     SNDFT,x
        sta     AUDF0
        lda     #8
        sta     AUDV0
        lda     #SNDLEN
        sta     SNDCNT
        rts

; SNDTICK -- call once a frame. Retires a cue when its frames run out.
SNDTICK: lda    SNDCNT
        beq     SNDT1
        dec     SNDCNT
        bne     SNDT1
        lda     #0
        sta     AUDV0
SNDT1:  rts

SNDCT:  DB      8, 12, 12, 8    ; waveform: a click, two tones, a low thud
SNDFT:  DB      20, 12, 6, 28   ; divider: pitch follows
