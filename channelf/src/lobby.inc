; ---------------------------------------------------------------------------
; lobby.inc -- ST_LOBBY: pick a table.
;
; One request. /tables returns TSTRIDE-byte records and the count is the reply
; length divided by the stride -- there is no count field. Rows are drawn
; straight out of the reply window, which is legal because the mailbox
; repaints it only on a SEQ commit and nothing here transacts between the
; fetch and the draw.
; ---------------------------------------------------------------------------

SLOBBY:	ENTER
	LI CVAL3
	LR 3,A
	DCI SLTITLE
	XDC
	PI UITITLE
	PI UICLRL
	LI CVAL1
	LR 3,A
	DCI SLWAIT
	XDC
	PI UIFOOT

	LI REQTBL
	DCI VREQ
	ST
	LIS 1
	DCI VMIN0
	ST
	CLR
	DCI VMIN1
	ST
	LI TMAXLEN&0FFH
	LR 0,A
	LI TMAXLEN>>8
	LR 1,A
	PI NFETCH
	CI 0
	BZ SLOK
	DCI VFAILC
	ST
	JMP FAIL

SLOK:	; The reply is a COUNT byte and then that many TSTRIDE-byte records --
	; the records start at offset 1, not 0. Dividing the reply length by the
	; stride looks equivalent and is not: 181 bytes is 1 + 5*36, and the
	; division says 5 only by accident of rounding.
	DCI FNREPLY
	LM
	LR 6,A
SLCNTX:	DCI VNTBL
	LR A,6
	ST
	DCI VTTOP
	CLR
	ST
	DCI VCUR
	CLR
	ST

	DCI VNTBL
	LM
	CI 0
	BNZ SLDRAW
	LI CVAL2
	LR 3,A
	CLR
	LR 0,A
	DCI SLNONE
	XDC
	PI UIROW
	LI CVAL1
	LR 3,A
	DCI SLRETRY
	XDC
	PI UIFOOT
SLNW:	PI INSCAN
	LR A,0
	CI EV_NONE
	BZ SLNW
	LI ST_LOBBY
	LR 0,A
	JMP GOTO

SLDRAW:	LI CVAL1
	LR 3,A
	DCI SLFOOT
	XDC
	PI UIFOOT
	PI SLROWS
	PI UICUR

SLIN:	PI INSCAN
	LR A,0
	CI EV_NONE
	BZ SLIN
	CI EV_DOWN
	BNZ SLI1
	JMP SLDOWN
SLI1:	CI EV_UP
	BNZ SLI2
	JMP SLUP
SLI2:	CI EV_BACK
	BNZ SLI3
	LI ST_EDIT
	LR 0,A
	JMP GOTO		; MODE re-types the name
SLI3:	CI EV_SEL
	BNZ SLIN
	JMP SLSEL

SLDOWN:	DCI VCUR
	LM
	LR 0,A
	INC
	LR 1,A
	LI NTBLROW
	LR 2,A
	LR A,1
	COM
	INC
	AS 2
	BM SLDSCR
	BZ SLDSCR
	DCI VTTOP
	LM
	AS 1
	LR 2,A
	DCI VNTBL
	LM
	COM
	INC
	AS 2			; (top+cur+1) - count
	BM SLDOK
	JMP SLIN
SLDOK:	LR A,1
	DCI VCUR
	ST
	PI UICUR
	JMP SLIN
SLDSCR:	DCI VTTOP
	LM
	AI NTBLROW
	LR 1,A
	DCI VNTBL
	LM
	COM
	INC
	AS 1			; (top+NTBLROW) - count
	BM SLDSC1
	JMP SLIN
SLDSC1:	DCI VTTOP
	LM
	INC
	DCI VTTOP
	ST
	JMP SLDRAW

SLUP:	DCI VCUR
	LM
	CI 0
	BNZ SLUOK
	DCI VTTOP
	LM
	CI 0
	BNZ SLUSCR
	JMP SLIN
SLUSCR:	DCI VTTOP
	LM
	AI 0FFH
	DCI VTTOP
	ST
	JMP SLDRAW
SLUOK:	AI 0FFH
	DCI VCUR
	ST
	PI UICUR
	JMP SLIN

; SLSEL -- take the highlighted table's id and sit down.
SLSEL:	PI SLREC		; DC1 = the record under the cursor
	PI SLSEL2
	LI ST_GAME
	LR 0,A
	JMP GOTO

; SLSEL2 -- copy the id at DC1 into VTABLE, NUL-terminated. Leaf.
SLSEL2:	DCI VTABLE		; DC0 = VTABLE (write), DC1 = the record (read)
	LI 9
	LR 4,A
SLS1:	XDC
	LM
	XDC
	ST
	CI 0
	BZ SLS2
	DS 4
	BNZ SLS1
SLS2:	DCI VTABLE+9
	CLR
	ST
	POP

; SLREC -- DC1 = the record for the row under the cursor. Leaf. Clobbers A, r4.
SLREC:	DCI VTTOP
	LM
	LR 4,A
	DCI VCUR
	LM
	AS 4
	LR 4,A
	DCI FNREPLY+1		; +1: byte 0 is the record count
	LR A,4
	CI 0
	BZ SLRX
SLR1:	LI TSTRIDE
	ADC
	DS 4
	BNZ SLR1
SLRX:	XDC
	POP

; SLROWS -- redraw the visible tables. Reached by PI; calls one level down.
SLROWS:	ENTER
	DCI VROW
	CLR
	ST
SLRW1:	DCI VROW
	LM
	CI NTBLROW
	BZ SLRWX
	DCI VROW
	LM
	LR 0,A
	PI UICLRR

	DCI VTTOP
	LM
	LR 4,A
	DCI VROW
	LM
	AS 4
	LR 5,A
	; draw when index < count. Negate the COUNT, not the index: adding the
	; index then gives index - count, and BM is "index is the smaller".
	DCI VNTBL
	LM
	COM
	INC
	AS 5			; index - count
	BM SLRWD
	BR SLRWN		; past the last table

SLRWD:	; DC0 = FNREPLY + 1 + index*TSTRIDE, at the name field
	LR A,5
	LR 4,A
	DCI FNREPLY+1+TNAME
	LR A,4
	CI 0
	BZ SLRWD2
SLRWD1:	LI TSTRIDE
	ADC
	DS 4
	BNZ SLRWD1
SLRWD2:	XDC
	LI CVAL3
	LR 3,A
	DCI VROW
	LM
	LR 0,A
	PI UIROW

SLRWN:	DCI VROW
	LM
	INC
	DCI VROW
	ST
	BR SLRW1
SLRWX:	LEAVE

SLTITLE: DB "GAME LOBBY",0
SLWAIT:	DB "FINDING TABLES...",0
SLFOOT:	DB "FIRE:SIT MODE:NAME",0
SLNONE:	DB "NO TABLES",0
SLRETRY: DB "ANY KEY: RETRY",0
