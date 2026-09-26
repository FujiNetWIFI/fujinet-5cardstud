; screens.inc -- table selection, the game screen, and the poll loop.
;
; Rendering policy: every poll redraws everything, opaquely, straight out
; of the reply window -- identical pixels rewritten are invisible, so
; there is no flicker and no digest bookkeeping. The one thing opaque
; overdraw cannot fix is content that SHRANK (a new hand dealt, a player
; gone), so CHKNEW watches round/playerCount and answers with a full
; clear + statics before the ordinary render.
;
; Seat geometry is the Lynx port's, compressed to 15 rows: master seats
; 0-7 run bottom-center, bottom-left, mid-left, top-left, top-center,
; top-right, mid-right, bottom-right; PCIDX picks which masters serve a
; given player count, with the requesting player (index 0) always at
; master seat 0. Hands are 11 cols wide worst case; every hand draws
; left-to-right whatever its side of the table.

; ======================================================================
; Table selection
; ======================================================================
TBLSCR: CALL    KEYRAW          ; a key still held from the previous
        LD      (V_KEY),A       ; screen must not fire here
        CALL    CLRSCR
        SYSSUK  STRDIS          ; two-line title, BIOS font at 2x
        DB      32
        DB      0
        DB      4CH
        DW      TFIVE
        SYSSUK  STRDIS
        DB      48
        DB      17
        DB      4CH
        DW      TSTUD
        LD      HL,TCHOOS
        LD      DE,6*256+2
        LD      C,XWONF
        CALL    TXTAT
TBLRF:  XOR     A               ; fetch /tables
        CALL    APICALL
        JR      NC,TBLOK
        LD      HL,ENET
        LD      DE,14*256+0
        LD      C,XRONF
        CALL    TXTAT
TBLRW:  CALL    KEYSCN
        OR      A
        JR      Z,TBLRW
        JR      TBLRF
TBLOK:  LD      HL,0
        CALL    RXGETB          ; table count
        OR      A
        JR      Z,TBLRW         ; none yet: wait for a key, refetch
        CP      9
        JR      C,TBOK2
        LD      A,8             ; eight rows fit under the title
TBOK2:  LD      (V_CNT),A
        XOR     A
        LD      (V_SEL),A
        CALL    TBDRAW
TBKEY:  CALL    KEYSCN
        OR      A
        JR      Z,TBKEY
        CP      'U'
        JR      NZ,TBK1
        LD      A,(V_SEL)
        OR      A
        JR      Z,TBKEY
        DEC     A
        LD      (V_SEL),A
        CALL    SNDCUR
        CALL    TBDRAW
        JR      TBKEY
TBK1:   CP      'D'
        JR      NZ,TBK2
        LD      A,(V_SEL)
        INC     A
        LD      HL,V_CNT
        CP      (HL)
        JR      NC,TBKEY
        LD      (V_SEL),A
        CALL    SNDCUR
        CALL    TBDRAW
        JR      TBKEY
TBK2:   CP      'E'             ; CE: back to the name screen
        JR      NZ,TBK2A
        CALL    NAMESCR
        JP      TBLSCR
TBK2A:  CP      '.'             ; the . key: how to play
        JR      NZ,TBK2B
        CALL    HELPSCR
        JP      TBLSCR
TBK2B:  CP      'T'
        JR      Z,TBJOIN
        CP      '0'             ; digit shortcut: 1-9 then 0 = row 10
        JR      C,TBKEY
        CP      '9'+1
        JR      NC,TBKEY
        SUB     '1'
        CP      0FFH            ; '0' wrapped to FF: row 10
        JR      NZ,TBK3
        LD      A,9
TBK3:   LD      HL,V_CNT
        CP      (HL)
        JR      NC,TBKEY
        LD      (V_SEL),A
; falls into the join
TBJOIN: CALL    SNDSEL
        LD      A,(V_SEL)       ; record base = 1 + sel*36
        CALL    TBOFF
        LD      DE,TBLBUF       ; id, up to its NUL
        LD      B,9
        CALL    RXSTRN
        LD      DE,9            ; the name field, for the game header
        ADD     HL,DE
        LD      DE,TBLNAM
        LD      B,21
        CALL    RXSTRN
        CALL    CLRSCR
        LD      HL,TJOIN
        LD      DE,7*256+14
        LD      C,XWONF
        CALL    TXTAT
        LD      A,0FFH          ; force the new-hand path on first poll
        LD      (PRVRND),A
        LD      (PRVACT),A      ; and arm the your-turn cue
        XOR     A
        LD      (PRVPC),A       ; no "player joined" for our own arrival
        LD      (V_MNU),A
        LD      (V_MUTE),A
        LD      H,A
        LD      L,A
        LD      (PRVPOT),HL
        LD      A,SQJOIN        ; sat down: cued once the table is drawn
        LD      (V_SNDQ),A
        JP      GAMELP

; TBOFF: HL = window offset of table record A (1 + A*36, and 36 = 32+4).
; Clobbers A,DE.
TBOFF:  LD      L,A
        LD      H,0
        ADD     HL,HL
        ADD     HL,HL           ; *4
        LD      E,L
        LD      D,H
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL           ; *32
        ADD     HL,DE           ; *36
        INC     HL              ; + the count byte
        RET

; TBDRAW: the list rows: name and "cur / max", selected row inverse.
TBDRAW: XOR     A
        LD      (V_TMP),A
TBD1:   LD      A,(V_TMP)
        LD      HL,V_CNT
        CP      (HL)
        RET     NC
        CALL    TBOFF
        PUSH    HL
        LD      DE,9            ; the display name field
        ADD     HL,DE
        LD      DE,LINBUF
        LD      B,21
        CALL    RXSTRN
        LD      A,(V_SEL)       ; row color
        LD      HL,V_TMP
        CP      (HL)
        LD      C,XFONW
        JR      Z,TBD2
        LD      C,XWONF
TBD2:   LD      A,(V_TMP)       ; row y = 7 + i
        ADD     A,7
        LD      D,A
        LD      E,2
        LD      HL,LINBUF
        LD      B,21
        PUSH    BC
        CALL    TXTN
        POP     BC
        POP     HL
        LD      DE,30           ; the "cur / max" field
        ADD     HL,DE
        LD      DE,LINBUF
        LD      B,6
        CALL    RXSTRN
        LD      A,(V_TMP)
        ADD     A,7
        LD      D,A
        LD      E,25
        LD      HL,LINBUF
        LD      B,6
        CALL    TXTN
        LD      A,(V_TMP)
        INC     A
        LD      (V_TMP),A
        JR      TBD1

; ======================================================================
; The game loop
; ======================================================================
GAMELP: LD      A,(MOVBUF)      ; a staged move rides the next request --
        OR      A               ; its reply IS the next state
        LD      A,1
        JR      Z,GLREQ
        LD      A,2
GLREQ:  CALL    APICALL
        JP      C,GLFAIL
        XOR     A
        LD      (MOVBUF),A      ; consumed
        CALL    CHKNEW
        CALL    RENDER
        XOR     A
        LD      (V_MUTE),A
        CALL    SNDQPL          ; the cues CHKNEW queued, now it is drawn
        LD      B,90            ; ~2 s of input scanning between polls
GLWAIT: PUSH    BC
        CALL    KNOBCK
        CALL    KEYSCN
        POP     BC
        OR      A
        JR      NZ,GLKEY
GLNEXT: LD      DE,2500         ; ~20 ms
GLDLY:  DEC     DE
        LD      A,D
        OR      E
        JR      NZ,GLDLY
        DJNZ    GLWAIT
        JR      GAMELP
GLKEY:  CP      'E'             ; CE leaves the table
        JR      NZ,GLK1
        LD      A,3
        CALL    APICALL         ; best effort; we are going regardless
        JP      TBLSCR
GLK1:   CP      '.'             ; how to play, then a forced full redraw
        JR      NZ,GLK1A
        CALL    HELPSCR
        LD      A,0FFH
        LD      (PRVRND),A
        LD      (V_MUTE),A      ; ...which is not a deal: no clicks
        JP      GAMELP
GLK1A:  LD      C,A             ; the hand controller works the menu,
        LD      A,(V_MNU)       ; while there is one
        OR      A
        JR      Z,GLK3
        LD      A,C
        CP      'L'
        JR      Z,GLLEFT
        CP      'R'
        JR      Z,GLRGHT
        CP      'T'
        JR      NZ,GLK3
        LD      A,(V_MSEL)      ; trigger: stage the highlighted move
        CALL    MVOFF
        CALL    RXGETB
        LD      (MOVBUF),A
        INC     HL
        CALL    RXGETB
        LD      (MOVBUF+1),A
        CALL    SNDSEL
GLK3:   JP      GAMELP          ; poll immediately (move or refresh)
GLLEFT: LD      A,(V_MSEL)
        OR      A
        JR      Z,GLBUMP
        DEC     A
        JR      GLMOVE
GLRGHT: LD      A,(V_MSEL)
        INC     A
        LD      HL,V_MNU
        CP      (HL)
        JR      NC,GLBUMP
GLMOVE: LD      (V_MSEL),A      ; the stick steps the cursor and stays in
        PUSH    BC              ; the wait: B still counts to the next poll
        CALL    SNDCUR
        CALL    BARMNU
        POP     BC
        JR      GLNEXT
GLBUMP: PUSH    BC
        CALL    SNDBAD
        POP     BC
        JR      GLNEXT

; KNOBCK: the knob picks a move by position -- the menu split into equal
; arcs. It only takes over once it has actually been turned (6 steps of
; hysteresis, which also rides out pot jitter), so the stick's choice is
; not yanked back to wherever the knob happens to rest. Clobbers all.
KNOBCK: LD      A,(V_MNU)
        OR      A
        RET     Z
        CALL    KNOBRD
        LD      C,A
        LD      HL,V_KNOB
        SUB     (HL)
        JR      NC,KNC1
        NEG
KNC1:   CP      6
        RET     C
        LD      (HL),C
        LD      A,C
        CALL    KNOBZN
        LD      HL,V_MSEL
        CP      (HL)
        RET     Z
        LD      (HL),A
        CALL    SNDCUR
        JP      BARMNU

GLFAIL: LD      HL,TBANG        ; a red mark in the corner; back off and
        LD      DE,0*256+39     ; try again
        LD      C,XRONF
        CALL    TXTAT
        LD      B,150
GLFW:   PUSH    BC
        CALL    KEYSCN
        POP     BC
        CP      'E'
        JR      NZ,GLFW1
        LD      A,3
        CALL    APICALL
        JP      TBLSCR
GLFW1:  LD      DE,2500
GLFD:   DEC     DE
        LD      A,D
        OR      E
        JR      NZ,GLFD
        DJNZ    GLFW
        JP      GAMELP

; CHKNEW: a new hand (round dropped) or a changed seat count means the
; screen holds cards that no longer exist: clear and redraw the statics.
; The same edges are the C clients' event cues -- a player arriving or
; leaving, the showdown, the bets swept in at a new round -- so they are
; queued here and played after the render.
CHKNEW: LD      E,0             ; E != 0: the screen needs a full clear
        LD      HL,GOPLCNT
        CALL    RXGETB
        LD      B,A             ; B = seats now
        LD      HL,PRVPC
        LD      C,(HL)          ; C = seats last poll
        LD      (HL),A
        CP      C
        JR      Z,CKRND
        INC     E
        LD      A,C             ; 0: our first poll here, not news
        OR      A
        JR      Z,CKRND
        LD      A,B
        CP      2               ; the C clients stay quiet below two
        JR      C,CKRND
        CP      C
        LD      A,SQPJN
        JR      NC,CKPC1
        LD      A,SQPLF
CKPC1:  CALL    SQADD
CKRND:  LD      HL,GOROUND
        CALL    RXGETB
        LD      HL,PRVRND
        LD      C,(HL)          ; C = round last poll (FFH = forced redraw)
        LD      (HL),A
        CP      C
        JR      NC,CKR1
        INC     E               ; dropped: a new hand
        JR      CKR9
CKR1:   JR      Z,CKR9          ; same round
        CP      5               ; a new round; the showdown gets a fanfare
        JR      NZ,CKR2
        LD      A,SQDONE
        CALL    SQADD
CKR2:   LD      A,C             ; out of round 0 there are no bets to sweep
        OR      A
        JR      Z,CKR9
        PUSH    DE
        LD      HL,GOPOT        ; the bets went in if the pot moved
        CALL    RXGETW
        LD      HL,(PRVPOT)
        LD      (PRVPOT),DE
        LD      A,D
        OR      E
        JR      Z,CKR3
        OR      A
        SBC     HL,DE
        JR      Z,CKR3
        LD      A,SQCHIP
        CALL    SQADD
CKR3:   POP     DE
CKR9:   LD      A,E
        OR      A
        RET     Z
; falls through into GSTATIC

; GSTATIC: clear + the parts of the game screen a poll never changes.
GSTATIC: CALL   CLRSCR
        XOR     A               ; the clear took every card with it
        LD      B,8
        LD      HL,PRVHND
GSTZ:   LD      (HL),A
        INC     HL
        DJNZ    GSTZ
        LD      HL,TBLNAM
        LD      DE,0*256+0
        LD      C,XWONF
        CALL    TXTAT
        LD      HL,TPOT
        LD      DE,6*256+18
        LD      C,XWONF
        JP      TXTAT

; ======================================================================
; Rendering a poll
; ======================================================================
RENDER: LD      HL,GOROUND      ; round indicator, top right
        CALL    RXGETB
        OR      A
        JR      NZ,RND1
        LD      HL,TWAIT
        JR      RNDDRW
RND1:   CP      5
        JR      C,RND2
        LD      HL,TSHOW
        JR      RNDDRW
RND2:   ADD     A,'0'
        LD      (LINBUF),A
        XOR     A
        LD      (LINBUF+1),A
        LD      HL,TROUND
        LD      DE,0*256+32
        LD      C,XWONF
        LD      B,6             ; pad to the digit, over WAITING leftovers
        CALL    TXTN
        LD      HL,LINBUF
        LD      DE,0*256+38
        LD      C,XWONF
        LD      B,2
        CALL    TXTN
        JR      RNDONE
RNDDRW: LD      DE,0*256+32
        LD      C,XWONF
        LD      B,8
        CALL    TXTN
RNDONE: LD      HL,GOPOT        ; the pot
        CALL    RXGETW
        EX      DE,HL
        CALL    U16STR
        LD      HL,LINBUF
        LD      DE,7*256+17
        LD      C,XWONF
        LD      B,5
        CALL    TXTN
        LD      HL,GOACTPL
        CALL    RXGETB
        LD      (V_ACT),A
        LD      HL,PRVACT       ; the your-turn cue, on the edge only
        CP      (HL)
        LD      (HL),A
        JR      Z,RENDA2
        OR      A
        JR      NZ,RENDA2
        LD      HL,GOMVCNT      ; a fresh turn: the cursor starts on the
        CALL    RXGETB          ; second move (call/check) when there is
        CP      2               ; one, as in the C clients
        LD      A,0
        JR      C,RENDA1
        INC     A
RENDA1: LD      (V_MSEL),A
        CALL    KNOBRD          ; the knob follows from where it rests
        LD      (V_KNOB),A
        LD      HL,GOMVTIM      ; the clock starts here: no tick for it
        CALL    RXGETB
        LD      (PRVTIM),A
        CALL    SNDTRN
RENDA2: LD      HL,GOPLCNT
        CALL    RXGETB
        LD      (V_PC),A
        XOR     A               ; each seated player
        LD      (V_TMP),A
RENDP:  LD      A,(V_TMP)
        LD      HL,V_PC
        CP      (HL)
        JR      NC,RENDG
        CALL    SEAT1
        LD      A,(V_TMP)
        INC     A
        LD      (V_TMP),A
        JR      RENDP
RENDG:  CALL    PURSE
        JP      BAR

; ----------------------------------------------------------------------
; SEAT1: render player (V_TMP): name, bet, hand.
SEAT1:  LD      A,(V_PC)        ; master seat = PCIDX[(pc-2)*8 + i]
        SUB     2
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      HL,V_TMP
        ADD     A,(HL)
        LD      HL,PCIDX
        LD      E,A
        LD      D,0
        ADD     HL,DE
        LD      A,(HL)
        LD      (V_SEAT),A
        LD      A,(V_TMP)       ; record base = GOPLYRS + i*33
        LD      L,A
        LD      H,0
        LD      E,A
        LD      D,H
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL           ; *32
        ADD     HL,DE           ; *33
        LD      DE,GOPLYRS
        ADD     HL,DE
        LD      (V_BASE),HL
        LD      DE,PLSTAT       ; status
        ADD     HL,DE
        CALL    RXGETB
        LD      (V_ST),A
        LD      HL,(V_BASE)     ; name
        LD      DE,LINBUF
        LD      B,9
        CALL    RXSTRN
        LD      A,(V_TMP)       ; color: active inverse, folded/left dim
        LD      HL,V_ACT
        CP      (HL)
        LD      C,XFONW
        JR      Z,SEATN
        LD      A,(V_ST)
        CP      2
        LD      C,XKONF
        JR      NC,SEATN
        LD      C,XWONF
SEATN:  LD      A,(V_SEAT)
        LD      HL,SEATNR
        CALL    TBLK
        LD      D,A
        LD      A,(V_SEAT)
        LD      HL,SEATNC
        CALL    TBLK
        LD      E,A
        LD      HL,LINBUF
        LD      B,9
        CALL    TXTN
        LD      A,(V_SEAT)      ; the bet chip; yours lives in the purse
        OR      A               ; line instead
        JR      Z,SEATH
        LD      HL,(V_BASE)
        LD      DE,PLBET
        ADD     HL,DE
        CALL    RXGETW
        LD      A,D
        OR      E
        JR      Z,SEATB0
        PUSH    DE
        CALL    SEATBP          ; D,E = bet row,col
        CALL    DRWCHP
        CALL    SEATBP
        INC     E               ; amount right of the chip
        POP     HL
        PUSH    DE
        CALL    U16STR
        POP     DE
        LD      HL,LINBUF
        LD      B,4
        LD      C,XWONF
        CALL    TXTN
        JR      SEATH
SEATB0: CALL    SEATBP          ; no bet: blank the chip cell + amount
        CALL    CRDPOS
        LD      B,6
        LD      C,1
        LD      D,0
        CALL    FILLRC
        CALL    SEATBP
        INC     E
        LD      HL,TNUL
        LD      B,4
        LD      C,XWONF
        CALL    TXTN
SEATH:  LD      HL,(V_BASE)     ; the hand
        LD      DE,PLHAND
        ADD     HL,DE
        LD      DE,HANDBUF
        LD      B,11
        CALL    RXSTRN
        CALL    HNDCNT          ; a click per card new since the last poll
        LD      A,(V_SEAT)
        LD      HL,PRVHND
        CALL    TBLK
        LD      C,A
        CALL    DEALCK
        LD      A,(V_SEAT)
        LD      HL,PRVHND
        LD      E,A
        LD      D,0
        ADD     HL,DE
        LD      A,(V_NCNT)
        LD      (HL),A
        LD      A,(V_SEAT)
        LD      HL,SEATHR
        CALL    TBLK
        LD      D,A
        LD      A,(V_SEAT)
        LD      HL,SEATHC
        CALL    TBLK
        LD      E,A
        LD      HL,HANDBUF
        LD      B,5
        JP      DRWHND

; DEALCK: a click per card that has just appeared -- (V_NCNT) now against
; C last poll -- which is the C clients' deal animation in sound.
; Preserves BC,DE.
DEALCK: LD      A,(V_MUTE)
        OR      A
        RET     NZ
        LD      A,(V_NCNT)
        SUB     C
        RET     Z
        RET     C
        PUSH    BC
        PUSH    DE
DLK1:   PUSH    AF
        CALL    SNDDEAL
        POP     AF
        DEC     A
        JR      NZ,DLK1
        POP     DE
        POP     BC
        RET

; HNDCNT: (V_NCNT) = cards in HANDBUF -- two wire chars per card, up to
; the NUL or space that also stops DRWHND. Clobbers A,B,HL.
HNDCNT: LD      HL,HANDBUF
        LD      B,0
HNC1:   LD      A,(HL)
        OR      A
        JR      Z,HNC2
        CP      ' '
        JR      Z,HNC2
        INC     HL
        INC     B
        JR      HNC1
HNC2:   LD      A,B
        SRL     A
        LD      (V_NCNT),A
        RET

; SEATBP: D,E = bet row,col of seat (V_SEAT). Clobbers A,HL.
SEATBP: LD      A,(V_SEAT)
        LD      HL,SEATBR
        CALL    TBLK
        LD      D,A
        LD      A,(V_SEAT)
        LD      HL,SEATBC
        CALL    TBLK
        LD      E,A
        RET

; TBLK: A = table(HL)[A]. Preserves DE (callers fill D and E from
; consecutive lookups).
TBLK:   PUSH    DE
        LD      E,A
        LD      D,0
        ADD     HL,DE
        LD      A,(HL)
        POP     DE
        RET

; ----------------------------------------------------------------------
; PURSE: row 13 -- your purse and bet (or the viewing banner), and the
; move clock at the right edge.
PURSE:  LD      HL,GOVIEW
        CALL    RXGETB
        OR      A
        JR      Z,PUR1
        LD      HL,TVIEW
        LD      DE,13*256+0
        LD      C,XWONF
        LD      B,22
        CALL    TXTN
        JR      PURT
PUR1:   LD      HL,TPURSE
        LD      DE,13*256+0
        LD      C,XWONF
        CALL    TXTAT
        LD      HL,GOPLYRS+PLPURSE
        CALL    RXGETW
        EX      DE,HL
        CALL    U16STR
        LD      HL,LINBUF
        LD      DE,13*256+6
        LD      C,XWONF
        LD      B,5
        CALL    TXTN
        LD      HL,TBET
        LD      DE,13*256+12
        LD      C,XWONF
        CALL    TXTAT
        LD      HL,GOPLYRS+PLBET
        CALL    RXGETW
        EX      DE,HL
        CALL    U16STR
        LD      HL,LINBUF
        LD      DE,13*256+16
        LD      C,XWONF
        LD      B,5
        CALL    TXTN
PURT:   LD      A,(V_ACT)       ; the move clock
        INC     A
        JR      Z,PURT0         ; nobody active
        LD      HL,GOMVTIM
        CALL    RXGETB
        LD      L,A
        LD      H,0
        CALL    U16STR
        LD      HL,LINBUF
        LD      DE,13*256+37
        LD      C,XRONF
        LD      B,2
        JP      TXTN
PURT0:  LD      HL,TNUL
        LD      DE,13*256+37
        LD      C,XWONF
        LD      B,2
        JP      TXTN

; ----------------------------------------------------------------------
; BAR: row 14, the white strip: your move menu when it is your turn,
; otherwise the last-result banner.
BAR:    LD      HL,NORMEM+14*240
        LD      B,6
        LD      C,BYTEPL
        LD      D,0FFH
        CALL    FILLRC
        LD      A,(V_ACT)
        OR      A
        JP      NZ,BARLR        ; not you (or nobody)
        LD      HL,GOVIEW
        CALL    RXGETB
        OR      A
        JP      NZ,BARLR
        LD      HL,GOMVCNT
        CALL    RXGETB
        OR      A
        JP      Z,BARLR
        CP      6               ; the wire says at most 5, but the cursor
        JR      C,BARMC         ; math trusts this: do not trust the wire
        LD      A,5
BARMC:  LD      (V_MNU),A
        LD      HL,GOMVTIM      ; the move clock, audibly, on your turn
        CALL    RXGETB
        LD      HL,PRVTIM
        CP      (HL)
        LD      (HL),A
        CALL    NZ,SNDTICK
; falls into BARMNU

; BARMNU: the (V_MNU) entries of the move menu, the one at V_MSEL
; inverse. No digits -- the stick or knob moves the highlight and the
; trigger bets it -- so each entry is a lead space, the name and a
; trailing space in one field, then a white gap. Also the stick/knob's
; redraw, which is why it stands alone. Clobbers everything.
BARMNU: LD      A,(V_MNU)
        LD      C,A
        LD      A,(V_MSEL)      ; the count can change under the cursor
        CP      C
        JR      C,BMN1
        LD      A,C
        DEC     A
        LD      (V_MSEL),A
BMN1:   LD      A,' '
        LD      (PADBUF),A
        XOR     A
        LD      (V_TMP2),A
        INC     A
        LD      (BARCOL),A
BARM:   LD      A,(V_TMP2)
        LD      HL,V_MNU
        CP      (HL)
        RET     NC
        CALL    MVOFF           ; the name: GOMOVES + n*13 + MVNAME
        LD      DE,MVNAME
        ADD     HL,DE
        LD      DE,LINBUF
        LD      B,10
        CALL    RXSTRN
        LD      A,(V_MSEL)
        LD      HL,V_TMP2
        CP      (HL)
        LD      C,XWONK
        JR      Z,BARM1
        LD      C,XKONW
BARM1:  LD      A,(BARCOL)
        LD      E,A
        LD      D,14
        LD      HL,LINBUF
        CALL    STRLEN
        ADD     A,2             ; lead space + name + trailing space
        LD      B,A
        PUSH    BC
        LD      HL,PADBUF
        CALL    TXTN
        POP     BC
        LD      A,B             ; advance: the field + a gap
        INC     A
        LD      HL,BARCOL
        ADD     A,(HL)
        LD      (HL),A
        LD      HL,V_TMP2
        INC     (HL)
        JR      BARM

; MVOFF: HL = window offset of move slot A (GOMOVES + A*13). Clobbers DE.
MVOFF:  LD      L,A
        LD      H,0
        LD      E,A
        LD      D,H
        ADD     HL,HL
        ADD     HL,DE           ; *3
        ADD     HL,HL
        ADD     HL,HL           ; *12
        ADD     HL,DE           ; *13
        LD      DE,GOMOVES
        ADD     HL,DE
        RET

; Not your turn. The server leaves lastResult holding the PREVIOUS hand's
; winner for the whole next hand, so showing it whenever the menu is down
; parks a stale banner on screen all hand long. Show it only between
; hands (round 0) and at the showdown; during a live hand, say whose
; turn it is.
BARLR:  XOR     A               ; no menu: the stick and knob rest
        LD      (V_MNU),A
        LD      HL,GOROUND
        CALL    RXGETB
        OR      A
        JR      Z,BARRES        ; between hands: the result stands
        CP      5
        JR      NC,BARRES       ; showdown: the result is the news
        LD      A,(V_ACT)
        INC     A               ; FFH = nobody: leave the bar blank
        RET     Z
        LD      HL,TWAITON
        LD      DE,14*256+1
        LD      C,XKONW
        CALL    TXTAT
        LD      A,(V_ACT)       ; the active player's name
        LD      L,A
        LD      H,0
        LD      E,A
        LD      D,H
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL           ; *32
        ADD     HL,DE           ; *33
        LD      DE,GOPLYRS
        ADD     HL,DE
        LD      DE,LINBUF
        LD      B,9
        CALL    RXSTRN
        LD      HL,LINBUF
        LD      DE,14*256+12
        LD      C,XKONW
        LD      B,9
        JP      TXTN
BARRES: LD      HL,GOLASTR      ; the last-result banner
        LD      DE,LINBUF
        LD      B,39
        CALL    RXSTRN
        LD      HL,LINBUF
        LD      DE,14*256+1
        LD      C,XKONW
        LD      B,39
        JP      TXTN

; ----------------------------------------------------------------------
; HELPSCR: how to play. Any key returns.
HELPSCR: CALL   KEYRAW
        LD      (V_KEY),A
        CALL    CLRSCR
        LD      HL,HELPTX
        LD      D,0             ; row; strings run in order, one per row,
HELP1:  LD      A,(HL)          ; first byte = column (FF ends)
        CP      0FFH
        JR      Z,HELP9
        LD      E,A
        INC     HL
        PUSH    DE
        PUSH    HL
        LD      C,XWONF
        CALL    TXTAT
        POP     HL
HELP2:  LD      A,(HL)          ; skip past the NUL
        INC     HL
        OR      A
        JR      NZ,HELP2
        POP     DE
        INC     D
        JR      HELP1
HELP9:  CALL    KEYSCN
        OR      A
        JR      Z,HELP9
        RET

HELPTX: DB      13,"HOW TO PLAY",0
        DB      0,"",0
        DB      1,"EACH PLAYER GETS ONE HOLE CARD",0
        DB      1,"THEN FOUR CARDS FACE UP, WITH",0
        DB      1,"BETTING AFTER EACH. THE BEST",0
        DB      1,"FIVE CARD HAND TAKES THE POT.",0
        DB      0,"",0
        DB      1,"STICK OR KNOB PICKS A MOVE",0
        DB      1,"TRIGGER BETS IT, JOINS A TABLE",0
        DB      1,"0 POLLS NOW    CE LEAVES",0
        DB      1,". SHOWS THIS SCREEN",0
        DB      0,"",0
        DB      1,"5CARD.CARR-DESIGNS.COM",0
        DB      0FFH

; STRLEN: A = length of the string at HL. Clobbers HL.
STRLEN: PUSH    BC
        LD      B,0
STRL1:  LD      A,(HL)
        OR      A
        JR      Z,STRL2
        INC     HL
        INC     B
        JR      STRL1
STRL2:  LD      A,B
        POP     BC
        RET

; ----------------------------------------------------------------------
; Seat geometry: master seats 0-7 (see the header comment).
SEATNC: DB      14,0,0,0,15,29,31,29    ; name col (31+9 is the screen edge)
SEATNR: DB      12,12,5,1,1,1,5,12      ; name row
SEATHC: DB      14,0,0,0,15,29,29,29    ; hand col
SEATHR: DB      9,9,6,2,2,2,6,9         ; hand row
SEATBC: DB      23,10,11,10,26,24,24,24 ; bet chip col (amount at +1)
SEATBR: DB      13,12,5,1,1,1,5,12      ; bet row

; playerCountIndex: which masters serve 2..8 players (Lynx vars.c,
; verbatim). The requesting player is always index 0 = master seat 0.
PCIDX:  DB      0,4,0,0,0,0,0,0
        DB      0,2,6,0,0,0,0,0
        DB      0,2,4,6,0,0,0,0
        DB      0,2,3,5,6,0,0,0
        DB      0,2,3,4,5,6,0,0
        DB      0,2,3,4,5,6,7,0
        DB      0,1,2,3,4,5,6,7

TCHOOS: DB      "CHOOSE A TABLE",0
TFIVE:  DB      "5 CARD",0
TSTUD:  DB      "STUD",0
TJOIN:  DB      "JOINING...",0
TPOT:   DB      "POT",0
TWAIT:  DB      "WAITING",0
TSHOW:  DB      "SHOWDOWN",0
TROUND: DB      "ROUND",0
TVIEW:  DB      "VIEWING - CE TO LEAVE",0
TPURSE: DB      "PURSE",0
TWAITON: DB     "WAITING ON",0
TBET:   DB      "BET",0
TBANG:  DB      "!",0
TNUL:   DB      0
