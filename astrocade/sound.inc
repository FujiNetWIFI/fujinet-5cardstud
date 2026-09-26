; sound.inc -- the shared client cue set, written straight to the music
; processor.
;
; Every cue in src/platform-specific/sound.h has a counterpart here, with
; pitches and lengths transcribed from the Intellivision port's sound.bas
; (itself taken from the C clients' Hz values). Tones are tone A alone;
; the deal click and the clock tick are the noise generator, as on the C64.
; Blocking is fine: every cue is a UI edge the C clients also stop for.
;
; Pitch (MAME's astrocde.cpp model): the master oscillator divides the
; 1.7897725 MHz clock by MO+1, each tone divides that by T+1 and toggles,
; so f = 1789773 / (2 * (MO+1) * (T+1)). With MO = 20H that is
; 27118 / (T+1); the 50-80 Hz player sweeps need the slow master,
; MO = FFH, f = 3496 / (T+1). Lengths are ~10 ms units (a 60 Hz frame
; from the references is 1.7 of them).

TONEA   EQU     11H             ; tone A period (TONMO+1)
SNDVIB  EQU     14H             ; vibrato; kept 0 so tones hold steady
MOHI    EQU     20H             ; master oscillator for the cues...
MOLO    EQU     0FFH            ; ...and for the low player sweeps

; Cue queue bits (V_SNDQ): set while the poll is examined, played by
; SNDQPL once RENDER has put the news on screen. Bit order = SQTAB order.
SQJOIN  EQU     01H
SQCHIP  EQU     02H
SQPJN   EQU     04H
SQPLF   EQU     08H
SQDONE  EQU     10H

; SQADD: queue cue bit A. Clobbers A,HL.
SQADD:  LD      HL,V_SNDQ
        OR      (HL)
        LD      (HL),A
        RET

; SNDQPL: play and empty the queue. Clobbers everything.
SNDQPL: LD      HL,V_SNDQ
        LD      A,(HL)
        LD      (HL),0
        LD      C,A
        LD      HL,SQTAB
SQP1:   LD      A,C
        OR      A
        RET     Z
        SRL     C
        JR      NC,SQP2
        PUSH    BC
        PUSH    HL
        LD      E,(HL)
        INC     HL
        LD      D,(HL)
        EX      DE,HL
        CALL    JPHL
        POP     HL
        POP     BC
SQP2:   INC     HL
        INC     HL
        JR      SQP1
JPHL:   JP      (HL)

SQTAB:  DW      SNDJOIN,SNDCHP,SNDPJN,SNDPLF,SNDDONE

; TONE: tone A at period A for B units, master oscillator C. Preserves
; C, HL; clobbers A,B,DE.
TONE:   PUSH    AF
        LD      A,C
        OUT     (TONMO),A
        XOR     A
        OUT     (SNDVIB),A
        POP     AF
        OUT     (TONEA),A
        LD      A,08H
        OUT     (VOLAB),A
        JR      GATE

; NOISE: a noise burst for B units. Register 5 bit 5 mixes the noise in
; (tone C left at volume 0), register 7's top nibble is its volume; bit 4
; stays clear so the noise never modulates the master oscillator.
; Preserves C, HL; clobbers A,B,DE.
NOISE:  LD      A,20H
        OUT     (VOLC),A
        LD      A,0A0H
        OUT     (VOLN),A
GATE:   CALL    WAITB
        XOR     A
        OUT     (VOLAB),A
        OUT     (VOLC),A
        OUT     (VOLN),A
        RET

; WAITB: B (1-255) x ~10 ms of busy wait. Clobbers A,B,DE.
WAITB:  LD      DE,600
WAITB1: DEC     DE
        LD      A,D
        OR      E
        JR      NZ,WAITB1
        DJNZ    WAITB
        RET

; PLAY: HL = notes of (period, length, rest after), ended by a 0 period;
; C = master oscillator. Clobbers A,B,DE,HL.
PLAY:   LD      A,(HL)
        OR      A
        RET     Z
        INC     HL
        LD      B,(HL)
        INC     HL
        CALL    TONE
        LD      B,(HL)
        INC     HL
        LD      A,B
        OR      A
        CALL    NZ,WAITB
        JR      PLAY

PLAYHI: LD      C,MOHI
        JR      PLAY

; soundJoinGame: you sit down (430, 340, 500 Hz).
SNDJOIN: LD     HL,FJOIN
        JR      PLAYHI

; soundMyTurn: a double beep (430 Hz twice).
SNDTRN: LD      HL,FTURN
        JR      PLAYHI

; soundGameDone: the showdown fanfare (311, 330, 392, 415 Hz).
SNDDONE: LD     HL,FDONE
        JR      PLAYHI

; soundSelectMove: a move, table or name confirmed (300 -> 350 Hz).
SNDSEL: LD      HL,FSEL
        JR      PLAYHI

; soundPlayerJoin / soundPlayerLeft: 50-80 Hz, rising / falling.
SNDPJN: LD      HL,FPJN
        JR      PLAYLO
SNDPLF: LD      HL,FPLF
PLAYLO: LD      C,MOLO
        JR      PLAY

; soundCursor: the cursor moved (300 Hz).
SNDCUR: LD      A,89
        JR      SNDBL

; soundCursorInvalid: the cursor hit an end (245 Hz, below the move blip).
SNDBAD: LD      A,110
SNDBL:  LD      B,3
        LD      C,MOHI
        JP      TONE

; soundDealCard: one card lands -- a noise click and a beat, so a run of
; them sounds like a deal rather than a buzz.
SNDDEAL: LD     B,3
        CALL    NOISE
        LD      B,5
        JP      WAITB

; soundTick: the move clock ticked on your turn.
SNDTICK: LD     B,2
        JP      NOISE

; soundTakeChip: the bets swept into the pot at a new street -- one clink
; per seat, each a little higher (the C clients' 0x800 + i*40 ramp).
SNDCHP: LD      A,(V_PC)
        OR      A
        RET     Z
        CP      9
        JR      C,SCHP0
        LD      A,8
SCHP0:  LD      H,A             ; H = seats, L = period (TONE keeps HL)
        LD      L,225           ; ~120 Hz
        LD      C,MOHI
SCHP1:  LD      A,L
        LD      B,3
        CALL    TONE
        LD      B,3
        CALL    WAITB
        LD      A,L
        SUB     12
        LD      L,A
        DEC     H
        JR      NZ,SCHP1
        RET

; Periods at MOHI: 430 Hz 62, 340 79, 500 53, 311 86, 330 81, 392 68,
; 415 64, 300 89, 350 76. At MOLO: 50 Hz 69, 60 57, 70 49, 80 43.
FJOIN:  DB      62,8,13, 79,8,0, 53,8,0, 0
FTURN:  DB      62,7,3, 62,7,3, 0
FDONE:  DB      86,17,0, 81,33,0, 68,17,0, 64,33,0, 0
FSEL:   DB      89,5,2, 76,5,0, 0
FPJN:   DB      69,3,25, 57,3,25, 49,3,25, 43,3,25, 0
FPLF:   DB      43,3,25, 49,3,25, 57,3,25, 69,3,25, 0
