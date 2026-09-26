; demo.inc -- static mock table, assembled when DEMO=1.
;
; M1 scaffolding: exercises every drawing path with zero network -- the
; screen sketch from the plan, populated with a full 8-player table. This
; is where the palette and card art get tuned by screenshot; the live
; screens (screens.inc) replace it from M3 on.

DEMOSCR:
        CALL    CLRSCR
; Row 0: table id left, round right.
        LD      HL,DTABLE
        LD      DE,0*256+0
        LD      C,XWONF
        CALL    TXTAT
        LD      HL,DROUND
        LD      DE,0*256+31
        LD      C,XWONF
        CALL    TXTAT

; Top seats (3, 4, 5): names row 1, hands rows 2-4. Seat 4 is the
; active player: inverse name.
        LD      HL,DNAME3
        LD      DE,1*256+0
        LD      C,XWONF
        CALL    TXTAT
        LD      HL,DNAME4
        LD      DE,1*256+15
        LD      C,XFONW         ; inverse: the active player
        CALL    TXTAT
        LD      HL,DNAME5
        LD      DE,1*256+29
        LD      C,XWONF
        CALL    TXTAT

        LD      HL,DHAND3
        LD      DE,2*256+0
        LD      B,5
        CALL    DRWHND
        LD      HL,DHAND4
        LD      DE,2*256+15
        LD      B,5
        CALL    DRWHND
        LD      HL,DHAND5
        LD      DE,2*256+29
        LD      B,5
        CALL    DRWHND

; Middle seats (2 left, 6 right): names row 5, hands rows 6-8.
        LD      HL,DNAME2
        LD      DE,5*256+0
        LD      C,XWONF
        CALL    TXTAT
        LD      HL,DNAME6
        LD      DE,5*256+32
        LD      C,XWONF
        CALL    TXTAT
        LD      HL,DHAND2
        LD      DE,6*256+0
        LD      B,5
        CALL    DRWHND
        LD      HL,DHAND6
        LD      DE,6*256+29
        LD      B,5
        CALL    DRWHND

; The pot, centered between the middle seats.
        LD      HL,DPOT
        LD      DE,6*256+16
        LD      C,XWONF
        CALL    TXTAT
        LD      HL,1250
        CALL    U16STR
        LD      HL,LINBUF
        LD      DE,7*256+17
        LD      C,XWONF
        CALL    TXTAT

; Bets: chips with amounts beside a few seats.
        LD      DE,5*256+11
        CALL    DRWCHP
        LD      HL,25
        CALL    U16STR
        LD      HL,LINBUF
        LD      DE,5*256+12
        LD      C,XWONF
        CALL    TXTAT
        LD      DE,9*256+25
        CALL    DRWCHP
        LD      HL,50
        CALL    U16STR
        LD      HL,LINBUF
        LD      DE,9*256+26
        LD      C,XWONF
        CALL    TXTAT

; Bottom seats (1 left, 0 you center, 7 right): hands rows 9-11, names
; row 12. Your hand is all face up.
        LD      HL,DHAND1
        LD      DE,9*256+0
        LD      B,5
        CALL    DRWHND
        LD      HL,DHAND0
        LD      DE,9*256+14
        LD      B,5
        CALL    DRWHND
        LD      HL,DHAND7
        LD      DE,9*256+29
        LD      B,5
        CALL    DRWHND
        LD      HL,DNAME1
        LD      DE,12*256+0
        LD      C,XWONF
        CALL    TXTAT
        LD      HL,DNAME0
        LD      DE,12*256+14
        LD      C,XWONF
        CALL    TXTAT
        LD      HL,DNAME7
        LD      DE,12*256+29
        LD      C,XWONF
        CALL    TXTAT

; Row 13: purse and the move clock.
        LD      HL,DPURSE
        LD      DE,13*256+0
        LD      C,XWONF
        CALL    TXTAT
        LD      HL,DCLOCK
        LD      DE,13*256+37
        LD      C,XRONF
        CALL    TXTAT

; Row 14: the status bar -- a white strip with the move menu in black.
        LD      HL,NORMEM+14*240
        LD      B,6
        LD      C,BYTEPL
        LD      D,0FFH
        CALL    FILLRC
        LD      HL,DMENU
        LD      DE,14*256+1
        LD      C,XKONW
        CALL    TXTAT
        LD      HL,DMSEL        ; the highlighted move, as BARMNU draws it
        LD      DE,14*256+8
        LD      C,XWONK
        JP      TXTAT

DTABLE: DB      "TABLE 1",0
DROUND: DB      "ROUND 3",0
DPOT:   DB      "POT",0
DPURSE: DB      "PURSE 340  BET 25",0
DCLOCK: DB      "12",0
DMENU:  DB      " FOLD   CALL   RAISE",0
DMSEL:  DB      " CALL ",0
DNAME0: DB      "YOU",0
DNAME1: DB      "MOZZWALD",0
DNAME2: DB      "ERICJ",0
DNAME3: DB      "THOM",0
DNAME4: DB      "JEFFP",0
DNAME5: DB      "ROGER",0
DNAME6: DB      "NORM",0
DNAME7: DB      "TSCHAK",0

DHAND0: DB      "astd9h2ckd",0
DHAND1: DB      "??qsjh7d",0
DHAND2: DB      "??2h5s9c",0
DHAND3: DB      "??tsth4d",0
DHAND4: DB      "??kcqd",0
DHAND5: DB      "??7c8s2d",0
DHAND6: DB      "??ah",0
DHAND7: DB      "??jdjc3s",0
