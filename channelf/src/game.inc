; ---------------------------------------------------------------------------
; game.inc -- ST_GAME: the table.
;
; Poll /state, draw, offer the moves the server says are legal, submit one.
; Everything is rendered straight out of the reply window between polls; the
; only things copied out are the table id and the player name, which have to
; survive the next request that overwrites the window.
;
; Row format, 22 cells after the cursor gutter:
;     NAME__ ppp CCCCCCCCCC
; six of name, three of purse, five cards as two ASCII bytes each. The server
; sends cards already as text ("as", "th"), so they are drawn as they arrive;
; "??" is a hole card.
; ---------------------------------------------------------------------------

; SGAME -- the poll loop.
SGAME:	ENTER
	LI CVAL3
	LR 3,A
	DCI SGWAIT
	XDC
	PI UITITLE
	PI UICLRL

SGPOLL:	LI REQSTAT
	DCI VREQ
	ST
	LI GMINLEN&0FFH
	DCI VMIN0
	ST
	LI GMINLEN>>8
	DCI VMIN1
	ST
	LI GMAXLEN&0FFH
	LR 0,A
	LI GMAXLEN>>8
	LR 1,A
	PI NFETCH
	CI 0
	BZ SGVAL
	DCI VFAILC
	ST
	JMP FAIL

	; Believe nothing until the length matches what playerCount implies:
	; 154 + 33 * players.
SGVAL:	DCI FNREPLY+GNPLAY
	LM
	LR 4,A			; players
	CLR
	LR 5,A			; running total, low
	CLR
	LR 6,A			; running total, high
	LI GMINLEN
	LR 5,A
SGVL1:	LR A,4
	CI 0
	BZ SGVL2
	LR A,5
	AI PSTRIDE
	LR 5,A
	BC SGVL1CY		; AI sets carry; that is the only carry-out there is
	BR SGVL1C
SGVL1CY: LR A,6
	INC
	LR 6,A
SGVL1C:	LR A,4
	AI 0FFH
	LR 4,A
	BR SGVL1
SGVL2:	DCI VRXLO
	LM
	XS 5
	BNZ SGBAD
	DCI VRXHI
	LM
	XS 6
	BNZ SGBAD
	BR SGDRAW
SGBAD:	LI 0EEH
	DCI VFAILC
	ST
	JMP FAIL

SGDRAW:	PI SGTITLE
	PI SGROWS
	PI SGFOOT

	; hold the picture for a moment, watching for input, then poll again
	LI 60
	DCI VPOLL
	ST
SGWAIT2: PI INSCAN
	LR A,0
	CI EV_NONE
	BNZ SGKEY
	LIS 8
	LR 3,A
	PI DELAYMS
	DCI VPOLL
	LM
	AI 0FFH
	DCI VPOLL
	ST
	CI 0
	BNZ SGWAIT2
	JMP SGPOLL

SGKEY:	CI EV_DOWN
	BNZ SGK1
	JMP SGMVDN
SGK1:	CI EV_UP
	BNZ SGK2
	JMP SGMVUP
SGK2:	CI EV_SEL
	BNZ SGK3
	JMP SGSEND
SGK3:	CI EV_BACK
	BNZ SGWAIT2
	JMP SGLEAVE

SGMVDN:	DCI FNREPLY+GNMOVE
	LM
	LR 4,A
	CI 0
	BZ SGWAIT2
	DCI VMVSEL
	LM
	INC
	LR 1,A
	LR A,4
	COM
	INC
	AS 1			; (sel+1) - count
	BM SGMVD1
	JMP SGWAIT2
SGMVD1:	LR A,1
	DCI VMVSEL
	ST
	PI SGFOOT
	JMP SGWAIT2

SGMVUP:	DCI VMVSEL
	LM
	CI 0
	BZ SGWAIT2
	AI 0FFH
	DCI VMVSEL
	ST
	PI SGFOOT
	JMP SGWAIT2

; SGSEND -- submit the highlighted move.
SGSEND:	DCI FNREPLY+GNMOVE
	LM
	CI 0
	BNZ SGSND1
	JMP SGWAIT2
SGSND1:	PI SGMVCODE		; VMOVE = the two-character code
	LI REQMOVE
	DCI VREQ
	ST
	LI GMINLEN&0FFH
	DCI VMIN0
	ST
	LI GMINLEN>>8
	DCI VMIN1
	ST
	LI GMAXLEN&0FFH
	LR 0,A
	LI GMAXLEN>>8
	LR 1,A
	PI NFETCH
	DCI VMVSEL
	CLR
	ST
	JMP SGPOLL

SGLEAVE: LI REQLEAVE
	DCI VREQ
	ST
	LIS 1
	DCI VMIN0
	ST
	CLR
	DCI VMIN1
	ST
	LI 32
	LR 0,A
	CLR
	LR 1,A
	PI NFETCH
	LI ST_LOBBY
	LR 0,A
	JMP GOTO

; ---- drawing ---------------------------------------------------------------

; SGTITLE -- "POT nnnn  R n". Reached by PI; calls one level down.
SGTITLE: ENTER
	DCI FNREPLY+GPOTLO
	LM
	LR 0,A
	DCI FNREPLY+GPOTHI
	LM
	LR 1,A
	PI DEC5
	DCI VLINE
	XDC
	DCI SGPOT
	PI SBCAT
	DCI VDIGIT
	PI SBCAT
	DCI SGRND
	PI SBCAT
	DCI FNREPLY+GROUND
	LM
	AI '0'
	LR 0,A
	PI SBCH
	PI SBEND
	LI CVAL3
	LR 3,A
	DCI VLINE
	XDC
	PI UITITLE
	LEAVE

; SGFOOT -- the highlighted valid move, or whose turn it is.
SGFOOT:	ENTER
	DCI FNREPLY+GNMOVE
	LM
	CI 0
	BNZ SGF1
	LI CVAL1
	LR 3,A
	DCI SGTHEIRS
	XDC
	PI UIFOOT
	LEAVE
SGF1:	PI SGMVNAME		; DC1 = the selected move's name
	LI CVAL2
	LR 3,A
	PI UIFOOT
	LEAVE

; SGMVOFF -- DC0 = FNREPLY + GMOVES + VMVSEL*GMVSTR. Leaf. Clobbers A, r4.
SGMVOFF: DCI VMVSEL
	LM
	LR 4,A
	DCI FNREPLY+GMOVES
	LR A,4
	CI 0
	BZ SGMOX
SGMO1:	LI GMVSTR
	ADC
	DS 4
	BNZ SGMO1
SGMOX:	POP

; SGMVNAME -- DC1 = the selected move's name (code[3] then name[10]). Leaf.
SGMVNAME: ENTER
	PI SGMVOFF
	LIS 3
	ADC
	XDC
	LEAVE

; SGMVCODE -- copy the selected move's code into VMOVE. Leaf-plus-one.
SGMVCODE: ENTER
	PI SGMVOFF
	XDC
	DCI VMOVE
	XDC
	LIS 3
	LR 4,A
SGMC1:	XDC
	LM
	XDC
	CI 0
	BZ SGMC2
	ST
	DS 4
	BNZ SGMC1
SGMC2:	DCI VMOVE+2
	CLR
	ST
	LEAVE

; SGROWS -- one row per player. Reached by PI; calls one level down.
SGROWS:	ENTER
	DCI VROW
	CLR
	ST
SGR1:	DCI VROW
	LM
	CI NPLRROW
	BZ SGRX
	DCI VROW
	LM
	LR 0,A
	PI UICLRR

	; draw when row < players; negate the COUNT so the sum is row - players
	DCI VROW
	LM
	LR 4,A
	DCI FNREPLY+GNPLAY
	LM
	COM
	INC
	AS 4			; row - players
	BM SGRD
	BR SGRN			; no player on this row

SGRD:	PI SGPLINE		; VLINE = the formatted row
	LI CVAL3
	LR 3,A
	DCI FNREPLY+GACTIVE
	LM
	LR 5,A
	DCI VROW
	LM
	XS 5
	BNZ SGRD2
	LI CVAL2
	LR 3,A			; the player to act gets the accent colour
SGRD2:	DCI VROW
	LM
	LR 0,A
	DCI VLINE
	XDC
	PI UIROW

SGRN:	DCI VROW
	LM
	INC
	DCI VROW
	ST
	BR SGR1
SGRX:	LEAVE

; SGPLINE -- build VLINE for the player on row VROW. Reached by PI; calls one
;   level down.
SGPLINE: ENTER
	; the purse first: DEC5 needs the record, and building the row will move
	; the cursor around
	PI SGPOFF
	LI PPURSLO
	ADC
	LM
	LR 0,A
	LM
	LR 1,A
	PI DEC5

	DCI VLINE
	XDC			; DC1 = the row cursor
	PI SGPOFF		; DC0 = the record, at the name
	LI 6
	LR 4,A
SGPL1:	LM
	CI 0
	BZ SGPL2
	XDC
	ST
	XDC
	DS 4
	BNZ SGPL1
	BR SGPL3
SGPL2:	LI ' '
	LR 0,A
SGPL2A:	PI SBCH
	DS 4
	BNZ SGPL2A
SGPL3:	LI ' '
	LR 0,A
	PI SBCH
	DCI VDIGIT
	PI SBCAT
	LI ' '
	LR 0,A
	PI SBCH

	PI SGPOFF
	LI PHAND
	ADC
	PI SBCAT
	PI SBEND
	LEAVE

; SGPOFF -- DC0 = FNREPLY + GPLAYER + VROW*PSTRIDE. Leaf. Clobbers A, r4.
SGPOFF:	DCI VROW
	LM
	LR 4,A
	DCI FNREPLY+GPLAYER
	LR A,4
	CI 0
	BZ SGPOX
SGPO1:	LI PSTRIDE
	ADC
	DS 4
	BNZ SGPO1
SGPOX:	POP

SGWAIT:	DB "SITTING DOWN...",0
SGPOT:	DB "POT ",0
SGRND:	DB "  R ",0
SGTHEIRS: DB "WAITING...",0
