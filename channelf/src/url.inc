; ---------------------------------------------------------------------------
; url.inc -- build the devicespec for a request.
;
; Streamed straight into the mailbox's TX byte stream a character at a time,
; the way the Odyssey 2 and Astrocade ports do it, and never assembled in RAM.
; Here that is a convenience rather than a necessity -- this console has 30K --
; but it keeps the request under one TX page without any length bookkeeping: a
; whole /move request is about 90 bytes against a 320-byte ceiling.
;
; Requests: 0 tables, 1 state, 2 move, 3 leave.
; ---------------------------------------------------------------------------

REQTBL	EQU 0
REQSTAT	EQU 1
REQMOVE	EQU 2
REQLEAVE EQU 3

; BLDURL -- append the devicespec for request VREQ. Reached by PI; calls one
;   level down (FNTXS). Clobbers A, r0, DC0, DC1.
BLDURL:	ENTER
	DCI UNPFX
	XDC
	PI FNTXS
	DCI UENDPT
	XDC
	PI FNTXS

	DCI VREQ
	LM
	CI REQTBL
	BNZ BU1
	DCI UTABLS
	XDC
	PI FNTXS
	LEAVE			; tables takes neither a table nor a player

BU1:	CI REQSTAT
	BNZ BU2
	DCI USTATE
	XDC
	PI FNTXS
	BR BUQ
BU2:	CI REQMOVE
	BNZ BU3
	DCI UMOVE
	XDC
	PI FNTXS
	DCI VMOVE		; the two-character move code
	XDC
	PI FNTXS
	BR BUQ
BU3:	DCI ULEAVE
	XDC
	PI FNTXS

BUQ:	DCI UQTAB
	XDC
	PI FNTXS
	DCI VTABLE
	XDC
	PI FNTXS
	DCI UQPLR
	XDC
	PI FNTXS
	DCI VNAME
	XDC
	PI FNTXS
	DCI UQBIN
	XDC
	PI FNTXS
	LEAVE

UNPFX:	DB "N:",0
UTABLS:	DB "tables?bin=1",0
USTATE:	DB "state",0
UMOVE:	DB "move/",0
ULEAVE:	DB "leave",0
UQTAB:	DB "?table=",0
UQPLR:	DB "&player=",0
UQBIN:	DB "&bin=1",0

	INCLUDE "endpoint.inc"
