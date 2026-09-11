; ---------------------------------------------------------------------------
; 5card.asm -- 5 Card Stud for the Fairchild Channel F.
;
; A standalone F8 client, in the Arcadia and Odyssey 2 mould: the shared C core
; cannot fit these machines, so the game is written to them.
;
; What is different here is that RAM is not the problem. The Arcadia client
; runs in 84 bytes and the O2 client in 39, because neither cartridge edge can
; accept a write cycle; this one can, so the cart hands the console 30K. The
; screens still render straight out of the mailbox's reply window rather than
; buffering the wire format -- not to save RAM, but because it is simpler and
; the reply-stability invariant makes it free.
;
; STRUCTURE. The F8 keeps one return address, so this is a flat state machine
; that JMPs between states, with calls at most two deep through the BIOS K
; stack (see f8call.inc).
; ---------------------------------------------------------------------------
	CPU F3850

ST_NAME	EQU 0			; fetch the shared username from its appkey
ST_EDIT	EQU 1			; type one, if the appkey was empty
ST_LOBBY EQU 2			; pick a table
ST_GAME	EQU 3			; play

NPLRROW	EQU 7			; player rows on screen
NTBLROW	EQU 7			; table rows on screen

; --- client RAM, in the cartridge's arena at $8000 ---
VHEX	EQU 08000H		; 3 bytes
VSTATE	EQU 08006H
VCUR	EQU 08007H		; cursor row
VNROW	EQU 08008H		; rows filled
VROW	EQU 0800DH		; draw loop counter
VPIXY	EQU 0800EH
VCY	EQU 08009H		; the seat row's top pixel row (cards.inc)
VCX	EQU 0800AH		; the current card slot's left pixel column
VCI	EQU 0800BH		; card slot 0..4
VIPCTL	EQU 08010H		; (input.inc)
VIPPAN	EQU 08011H
VREQ	EQU 08012H		; which request BLDURL should build
VAVLO	EQU 08013H		; bytes the adapter says are waiting
VAVHI	EQU 08014H
VPRVLO	EQU 08015H		; the previous STATUS reading
VPRVHI	EQU 08016H
VNTRY	EQU 08017H		; settle-poll countdown
VRXLO	EQU 08018H		; reply length of the last READ
VRXHI	EQU 08019H
VMIN0	EQU 08026H		; bytes NSETTLE must see before it believes a reply
VMIN1	EQU 08027H
VFAILC	EQU 08028H		; the code FAIL shows
VCRNK	EQU 08029H		; the card's rank byte, off the wire
VCSUI	EQU 0802AH		; its suit -- MUST be VCRNK+1: SGHAND stores both
				; after a single DCI
VCCOL	EQU 0802BH		; the suit's colour, carried across two fills
VCEND	EQU 0802CH		; the hand's NUL has been reached
VMVI	EQU 0802DH		; the move SGMVOFF indexes, 0..4
VMVCOL	EQU 0802EH		; the footer's running cell
VMVLEN	EQU 0802FH		; characters of the move name actually drawn
VMVBUF	EQU 080C0H		; one move name, clipped to the line
VNTBL	EQU 0801AH		; tables the lobby found
VTTOP	EQU 0801BH		; first table shown
VEDX	EQU 0801CH		; keyboard cursor
VEDY	EQU 0801DH
VEDLEN	EQU 0801EH
VEDMAX	EQU 0801FH
VEDCASE	EQU 08020H
VEDDR	EQU 08021H		; keyboard draw counters
VEDDC	EQU 08022H
VMVSEL	EQU 08023H		; selected move
VNMOVE	EQU 08024H		; valid moves offered
VPOLL	EQU 08025H		; frames until the next /state poll
VEDCH	EQU 08030H		; one character plus a NUL, for drawing a cell
VMOVE	EQU 08040H		; the two-character move code, NUL-terminated
VTABLE	EQU 08050H		; table id
VNAME	EQU 08060H		; player name
VENTRY	EQU 08080H		; editor accumulator
VLINE	EQU 08100H		; one formatted row
VDIGIT	EQU 08140H		; a formatted number

	ORG 0800H
	DB 55H			; cart signature -- the BIOS compares this at $0800
	DB 00H			; $0801 is skipped; the BIOS jumps to $0802

ENTRY:	DI

	LI CVAL0
	LR 3,A
	PI BCLRSCR
	LI PAL2A
	LR 3,A
	LI PAL2B
	LR 4,A
	PI DPAL

	DCI VSTATE
	CLR
	ST
	DCI VTABLE
	CLR
	ST
	DCI VNAME
	CLR
	ST
	DCI VMOVE
	CLR
	ST

	PI FNCHK
	BZ MAIN
	LI CVAL2
	LR 3,A
	LI DORGX
	LR 1,A
	LI DORGY+3*DCELLH
	LR 2,A
	DCI SNOCART
	PI DSTR
HLTNC:	BR HLTNC

MAIN:	DCI VSTATE
	LM
	CI ST_NAME
	BNZ MD1
	JMP SNAME
MD1:	CI ST_EDIT
	BNZ MD2
	JMP SEDIT
MD2:	CI ST_LOBBY
	BNZ MD3
	JMP SLOBBY
MD3:	CI ST_GAME
	BNZ MAIN
	JMP SGAME

GOTO:	DCI VSTATE
	LR A,0
	ST
	JMP MAIN

; FAIL -- show a code on the footer and wait for a key, then back to the lobby.
;   Reached by JMP with the code already in VFAILC. It cannot live in a
;   register: UIFOOT writes r0-r8 on its way to drawing the label, so a code
;   parked in r1 comes back as part of a screen coordinate.
FAIL:	LI CVAL2
	LR 3,A
	DCI SERR
	XDC
	PI UIFOOT
	DCI VFAILC
	LM
	LR 0,A			; DHEXS takes its byte in r0
	DCI VHEX
	PI DHEXS
	DCI VHEX
	PI DSTR
FAILW:	PI INSCAN
	LR A,0
	CI EV_NONE
	BZ FAILW
	LI ST_LOBBY
	LR 0,A
	JMP GOTO

	INCLUDE "f8call.inc"
	INCLUDE "fujidisp.inc"
	INCLUDE "ui.inc"
	INCLUDE "fujilib.inc"
	INCLUDE "input.inc"
	INCLUDE "strings.inc"
	INCLUDE "state.inc"
	INCLUDE "url.inc"
	INCLUDE "net.inc"
	INCLUDE "nament.inc"
	INCLUDE "lobby.inc"
	INCLUDE "game.inc"
	INCLUDE "cards.inc"
	INCLUDE "font.inc"

SNOCART: DB "NO FUJINET CART",0
SERR:	DB "NET ERR ",0

	ORG 0800H+FN_ROM_CLAIM
	DB "FUJI"

	END
