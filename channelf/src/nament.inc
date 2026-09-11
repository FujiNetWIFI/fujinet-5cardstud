; ---------------------------------------------------------------------------
; nament.inc -- the player name: fetch it, or type it.
;
; ST_NAME reads the username every client in this family shares -- the appkey
; at creator 1, app 1, key 0 -- and only falls through to the keyboard when
; that comes back empty. Typing it once on any console is enough for all of
; them.
;
; The keyboard itself is the CONFIG port's, unchanged: four rows of sixteen
; characters plus CASE/SPC/DEL/OK/ESC, with the selected cell drawn in the
; accent colour rather than inverted, because inverting needs VRAM readback and
; this console has none.
; ---------------------------------------------------------------------------

EDKROW	EQU 2			; first list row of the keyboard
EDCROW	EQU 6			; the control row
EDNCTL	EQU 5			; CASE SPC DEL OK ESC
NAMEMAX	EQU 8			; the wire field is name[9] including its NUL

; ---------------------------------------------------------------------------
; SNAME -- read the shared username from its appkey.
; ---------------------------------------------------------------------------
SNAME:	ENTER
	LI CVAL3
	LR 3,A
	DCI SNTITLE
	XDC
	PI UITITLE
	PI UICLRL
	LI CVAL1
	LR 3,A
	DCI SNWAIT
	XDC
	PI UIFOOT

	PI AKOPEN
	CI 0
	BNZ SNTYPE
	PI AKREAD
	CI 0
	BNZ SNTYPE
	; the reply is a length word then the value; an empty key gives 0
	DCI FNREPLY
	LM
	CI 0
	BZ SNTYPE
	DCI FNREPLY+2
	XDC
	DCI VNAME
	XDC
	LI NAMEMAX
	LR 4,A
SNCP:	LM
	XDC
	ST
	XDC
	CI 0
	BZ SNGO
	DS 4
	BNZ SNCP
SNGO:	DCI VNAME+NAMEMAX
	CLR
	ST
	DCI VNAME
	LM
	CI 0
	BZ SNTYPE
	LI ST_LOBBY
	LR 0,A
	JMP GOTO

SNTYPE:	DCI VENTRY
	CLR
	ST
	LI ST_EDIT
	LR 0,A
	JMP GOTO

; AKOPEN -- OPEN_APPKEY(creator, app, key, read mode).
AKOPEN:	ENTER
	LI FD_FUJI
	LR 0,A
	LI FC_AKOPN
	LR 1,A
	LIS 4
	LR 2,A
	PI FNBEG
	LI AKCREAT
	LR 0,A
	PI FNPB
	LI AKAPP
	LR 0,A
	PI FNPB
	LI AKKEY
	LR 0,A
	PI FNPB
	CLR
	LR 0,A
	PI FNPB
	PI FNGO
	PI FNACK
	LEAVE

AKREAD:	ENTER
	LI FD_FUJI
	LR 0,A
	LI FC_AKRD
	LR 1,A
	CLR
	LR 2,A
	PI FNBEG
	PI FNGO
	PI FNACK
	LEAVE

; AKWRITE -- WRITE_APPKEY with VNAME. The payload is a length word then the
;   value, which is what READ_APPKEY hands back.
AKWRITE: ENTER
	LI FD_FUJI
	LR 0,A
	LI FC_AKWR
	LR 1,A
	LIS 1
	LR 2,A
	PI FNBEG
	DCI VEDLEN
	LM
	LR 0,A
	PI FNPB
	DCI VNAME
	XDC
	LI NAMEMAX+1
	LR 0,A
	PI FNFIX
	PI FNGO
	PI FNACK
	LEAVE

; ---------------------------------------------------------------------------
; SEDIT -- type a name.
; ---------------------------------------------------------------------------
SEDIT:	ENTER
	DCI VEDX
	CLR
	ST
	CLR
	ST			; VEDY
	DCI VEDCASE
	CLR
	ST

	; length of whatever was seeded into the accumulator
	DCI VENTRY
	CLR
	LR 0,A
SEDLEN:	LM
	CI 0
	BZ SEDLEN2
	LR A,0
	INC
	LR 0,A
	BR SEDLEN
SEDLEN2: DCI VEDLEN
	LR A,0
	ST

	LI NAMEMAX
	DCI VEDMAX
	ST
	LI CVAL3
	LR 3,A
	DCI SEDTITLE
	XDC
	PI UITITLE
	LI CVAL1
	LR 3,A
	DCI SEDFOOT
	XDC
	PI UIFOOT

SEDDRW:	PI EDDRAW

SEDIN:	PI INSCAN
	LR A,0
	CI EV_NONE
	BZ SEDIN
	CI EV_UP
	BNZ SED1
	JMP SEDUP
SED1:	CI EV_DOWN
	BNZ SED2
	JMP SEDDN
SED2:	CI EV_LEFT
	BNZ SED3
	JMP SEDLT
SED3:	CI EV_PGUP		; TIME, the same move without the stick
	BNZ SED4
	JMP SEDLT
SED4:	CI EV_RIGHT
	BNZ SED5
	JMP SEDRT
SED5:	CI EV_PGDN		; HOLD
	BNZ SED6
	JMP SEDRT
SED6:	CI EV_SEL
	BNZ SED7
	JMP SEDPIK
SED7:	CI EV_BACK
	BNZ SEDIN
	JMP SEDDEL

SEDUP:	DCI VEDY
	LM
	CI 0
	BZ SEDIN
	AI 0FFH
	DCI VEDY
	ST
	PI EDCLAMP
	JMP SEDDRW

SEDDN:	DCI VEDY
	LM
	CI 4
	BZ SEDIN
	INC
	DCI VEDY
	ST
	PI EDCLAMP
	JMP SEDDRW

SEDLT:	DCI VEDX
	LM
	CI 0
	BZ SEDIN
	AI 0FFH
	DCI VEDX
	ST
	JMP SEDDRW

SEDRT:	DCI VEDX
	LM
	LR 0,A
	PI EDWIDE		; r1 = cells on this row
	LR A,0
	INC
	LR 2,A
	LR A,1
	COM
	INC
	AS 2			; (x+1) - width
	BM SEDRTO
	JMP SEDIN
SEDRTO:	LR A,0
	INC
	DCI VEDX
	ST
	JMP SEDDRW

; SEDDEL -- MODE is a shortcut for the DEL cell.
SEDDEL:	PI EDDEL
	JMP SEDDRW

SEDPIK:	DCI VEDY
	LM
	CI 4
	BZ SEDCTL

	; a character cell: index = y*16 + x
	DCI VEDY
	LM
	LR 4,A
	CLR
	LR 5,A
SEDPM:	LR A,4
	CI 0
	BZ SEDPM2
	LR A,5
	AI 16
	LR 5,A
	LR A,4
	AI 0FFH
	LR 4,A
	BR SEDPM
SEDPM2:	DCI VEDX
	LM
	AS 5
	LR 0,A
	DCI EDCHARS
	LR A,0
	ADC
	LM
	LR 0,A			; r0 = the character
	DCI VEDCASE
	LM
	CI 0
	BZ SEDPINS
	LR A,0
	CI 61H
	BP SEDPINS		; $61 >= A: not a lowercase letter
	CI 7AH
	BM SEDPINS
	AI 0E0H			; -$20: uppercase
	LR 0,A
SEDPINS: PI EDINS
	JMP SEDDRW

SEDCTL:	DCI VEDX
	LM
	CI 0
	BNZ SEDC1
	; CASE
	DCI VEDCASE
	LM
	COM
	NI 1
	DCI VEDCASE
	ST
	JMP SEDDRW
SEDC1:	CI 1
	BNZ SEDC2
	LI ' '
	LR 0,A
	PI EDINS
	JMP SEDDRW
SEDC2:	CI 2
	BNZ SEDC3
	PI EDDEL
	JMP SEDDRW
SEDC3:	CI 3
	BNZ SEDC4
	JMP SEDOK
SEDC4:	JMP SEDESC

; SEDOK -- keep the name, remember it for every other console, and play.
SEDOK:	DCI VENTRY
	XDC
	DCI VNAME
	XDC
SEDOKC:	LM
	XDC
	ST
	XDC
	CI 0
	BNZ SEDOKC
	PI AKWRITE		; best effort: an unwritable appkey is not fatal
	LI ST_LOBBY
	LR 0,A
	JMP GOTO

SEDESC:	LI ST_LOBBY
	LR 0,A
	JMP GOTO

; ---- helpers ---------------------------------------------------------------

; EDSTORE -- copy VENTRY to the buffer at DC1, NUL-terminated. Leaf.
EDSTORE: DCI VENTRY
EDST1:	LM
	XDC
	ST
	XDC
	CI 0
	BNZ EDST1
	POP

; EDINS -- append r0 to the accumulator if there is room. Leaf.
EDINS:	DCI VEDLEN
	LM
	LR 1,A
	; negate the MAX and add the length: the sum is len - max, and BM then
	; means "there is room". Negating the length instead computes max - len
	; and inverts the test while the comment still reads true.
	DCI VEDMAX
	LM
	COM
	INC
	AS 1			; len - max
	BM EDINS1
	POP			; full
EDINS1:	DCI VENTRY
	LR A,1
	ADC
	LR A,0
	ST
	CLR
	ST
	DCI VEDLEN
	LR A,1
	INC
	ST
	POP

; EDDEL -- drop the last character. Leaf.
EDDEL:	DCI VEDLEN
	LM
	CI 0
	BNZ EDDEL1
	POP
EDDEL1:	AI 0FFH
	LR 1,A
	DCI VEDLEN
	LR A,1
	ST
	DCI VENTRY
	LR A,1
	ADC
	CLR
	ST
	POP

; EDWIDE -- r1 = how many cells row VEDY has. Leaf.
EDWIDE:	DCI VEDY
	LM
	CI 4
	BZ EDWC
	LI 16
	LR 1,A
	POP
EDWC:	LI EDNCTL
	LR 1,A
	POP

; EDCLAMP -- keep VEDX inside the row it just moved to. Leaf-ish; calls EDWIDE,
;   so it is reached by PI and uses ENTER/LEAVE.
EDCLAMP: ENTER
	PI EDWIDE
	DCI VEDX
	LM
	LR 0,A
	LR A,1
	COM
	INC
	AS 0			; x - width
	BM EDCLX
	LR A,1
	AI 0FFH
	DCI VEDX
	ST
EDCLX:	LEAVE

; EDDRAW -- repaint the accumulator, the keyboard and the control row.
;
;   Every loop counter is in RAM. DSTR and DCLRR between them write r0 and
;   r2-r8, which is the whole directly addressable file bar r1 -- an earlier
;   version kept the row and column in r6/r7 and drew exactly one cell before
;   losing its place.
;   Reached by PI; calls one level down.
EDDRAW:	ENTER
	CLR
	LR 0,A
	PI UICLRR
	LI CVAL2
	LR 3,A
	CLR
	LR 0,A
	DCI VENTRY
	XDC
	PI UIROW
	LIS 1
	LR 0,A
	PI UICLRR		; the spacer row, or the screen underneath shows through

	DCI VEDDR
	CLR
	ST
EDDR1:	DCI VEDDR
	LM
	CI 4
	BNZ EDDR1A
	JMP EDDRC		; the four keyboard rows are past a branch's reach
EDDR1A:
	DCI VEDDR
	LM
	AI EDKROW
	LR 0,A
	PI UICLRR

	DCI VEDDC
	CLR
	ST
EDDR2:	DCI VEDDC
	LM
	CI 16
	BNZ EDDR2A
	JMP EDDR3		; a cell draw is past a relative branch's reach
EDDR2A:

	; character index = row*16 + column
	DCI VEDDR
	LM
	LR 4,A
	CLR
	LR 5,A
EDDRM:	LR A,4
	CI 0
	BZ EDDRM2
	LR A,5
	AI 16
	LR 5,A
	LR A,4
	AI 0FFH
	LR 4,A
	BR EDDRM
EDDRM2:	DCI VEDDC
	LM
	AS 5
	LR 0,A
	DCI EDCHARS
	LR A,0
	ADC
	LM
	LR 4,A			; r4 = the character
	DCI VEDCASE
	LM
	CI 0
	BZ EDDRPUT
	LR A,4
	CI 61H
	BP EDDRPUT		; $61 >= A: not a lowercase letter
	CI 7AH
	BM EDDRPUT
	AI 0E0H
	LR 4,A
EDDRPUT: DCI VEDCH
	LR A,4
	ST
	CLR
	ST

	; accent colour when this cell is the cursor
	LI CVAL3
	LR 3,A
	DCI VEDY
	LM
	LR 4,A
	DCI VEDDR
	LM
	XS 4
	BNZ EDDRP2
	DCI VEDX
	LM
	LR 4,A
	DCI VEDDC
	LM
	XS 4
	BNZ EDDRP2
	LI CVAL2
	LR 3,A
EDDRP2:	DCI VEDDR
	LM
	AI EDKROW
	LR 0,A
	PI UIROWY
	DCI VEDDC
	LM
	SL 1
	SL 1
	AI DORGX+DCELLW
	LR 1,A
	DCI VEDCH
	PI DSTR

	DCI VEDDC
	LM
	INC
	DCI VEDDC
	ST
	JMP EDDR2		; a cell draw is far more than a branch can reach

EDDR3:	DCI VEDDR
	LM
	INC
	DCI VEDDR
	ST
	JMP EDDR1

	; the control row
EDDRC:	LI EDCROW
	LR 0,A
	PI UICLRR
	DCI VEDDC
	CLR
	ST
EDDRC1:	DCI VEDDC
	LM
	CI EDNCTL
	BZ EDDRCX

	LI CVAL3
	LR 3,A
	DCI VEDY
	LM
	CI 4
	BNZ EDDRC2
	DCI VEDX
	LM
	LR 4,A
	DCI VEDDC
	LM
	XS 4
	BNZ EDDRC2
	LI CVAL2
	LR 3,A
EDDRC2:	LI EDCROW
	LR 0,A
	PI UIROWY
	DCI VEDDC
	LM
	LR 4,A
	CLR
	LR 5,A
EDDRCM:	LR A,4
	CI 0
	BZ EDDRCM2
	LR A,5
	AI 5*DCELLW
	LR 5,A
	LR A,4
	AI 0FFH
	LR 4,A
	BR EDDRCM
EDDRCM2: LR A,5
	AI DORGX
	LR 1,A
	DCI VEDDC
	LM
	LR 0,A
	PI EDCTLS		; leaves the label in DC0, which is where DSTR reads
	PI DSTR

	DCI VEDDC
	LM
	INC
	DCI VEDDC
	ST
	JMP EDDRC1
EDDRCX:	LEAVE

; EDCTLS -- DC1 = the label for control cell r0. Leaf.
EDCTLS:	LR A,0
	CI 0
	BNZ EDCT1
	DCI SEDCASE
	POP
EDCT1:	CI 1
	BNZ EDCT2
	DCI SEDSPC
	POP
EDCT2:	CI 2
	BNZ EDCT3
	DCI SEDDELS
	POP
EDCT3:	CI 3
	BNZ EDCT4
	DCI SEDOKS
	POP
EDCT4:	DCI SEDESCS
	POP


EDCHARS: DB "abcdefghijklmnop"
	DB "qrstuvwxyz012345"
	DB "6789.-_/:;!?@#$%"
	DB "&*()+=[]{}<>,'~ "

SEDTITLE: DB "YOUR NAME",0
SEDCASE: DB "CASE",0
SEDSPC:	DB "SPC",0
SEDDELS: DB "DEL",0
SEDOKS:	DB "OK",0
SEDESCS: DB "ESC",0
SEDFOOT: DB "FIRE:PICK MODE:DEL",0
SNTITLE: DB "5 CARD STUD",0
SNWAIT:  DB "CHECKING NAME...",0
