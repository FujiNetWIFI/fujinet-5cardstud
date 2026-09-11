; ---------------------------------------------------------------------------
; cards.inc -- card faces on felt.
;
; This machine has one palette for the whole screen and no white in it, so the
; lightest thing available is the background itself. The card face is therefore
; not drawn: it is what is left when the felt is painted around it.
;
; Five cards on an 11-pixel pitch, each slot a 1-pixel felt column and a
; 10-pixel face:
;
;     39 40......49 50 51......60 61 ... 83 84......93 94
;     |f |card 0   |f |card 1   |f | ... |f |card 4   |f
;
; and 5 pixels tall, which leaves the row's blank sixth pixel row to be painted
; felt as well -- so every card is closed on all four sides for nothing. The
; separators run the full 6 pixels, so they join up into continuous columns
; down the table; SGTITLE caps the top of seat 0 the same way.
;
; NOTHING HERE IS WRITTEN TWICE. The separator, the face and the felt row under
; it are disjoint, and SGROWS clears only the 9 text cells rather than the whole
; row -- 216 + 336 is exactly the 552 pixels the old full-width clear cost, so
; the table gains card art for no redraw time at all. Filling the whole bed with
; felt and punching the faces back out would have cost 45% more.
;
; A face is two stamps: a 5-wide rank then a 5x5 suit pip. Hearts and diamonds
; draw RED, spades and clubs BLUE -- the closest thing to black in this palette.
; A hole card is a red lattice across the whole face; an undealt slot is bare
; felt, so how many cards are out reads at a glance.
; ---------------------------------------------------------------------------

CDX0	EQU 39			; the felt column left of card 0
CDW	EQU 10			; face width
CDH	EQU 5			; face height
CDPIT	EQU 11			; separator plus face
CDRW	EQU 5			; rank field width; the pip follows it
CDBEDW	EQU 5*CDPIT+1		; 56 -- the whole bed, both end columns included
SGTXTW	EQU (CDX0+1-DORGX)/DCELLW  ; 9 cells: what is left for name and purse

; ---------------------------------------------------------------------------
; SGHAND -- the five card slots of the player on row VROW.
;   VCY holds the row's top pixel row. Reached by PI; calls two levels down.
; ---------------------------------------------------------------------------
SGHAND:	ENTER
	LI CDX0
	DCI VCX
	ST
	CLR
	DCI VCI
	ST
	CLR
	DCI VCEND
	ST

	; The slot index and its pixel column live in RAM: DFILL and DSTAMP
	; between them write r0 and r4-r8, and ENTER/LEAVE write r7 and A.
SGH1:	DCI VCI
	LM
	CI 5
	BZ SGHX
	SL 1
	LR 5,A			; 2*i -- SGPOFF clobbers only A and r4
	PI SGPOFF		; DC0 = the player's record
	LR A,5
	AI PHAND
	ADC			; PHAND + 2*i, at most 30: one signed ADC
	LM
	LR 5,A			; rank
	LM
	LR 6,A			; suit

	; Once a NUL has been seen every later slot is empty too. The hand is a
	; C string -- the reference client walks it with strlen -- so whatever
	; follows the terminator is not defined to be anything, and believing a
	; later slot's byte would draw it.
	DCI VCEND
	LM
	CI 0
	BNZ SGH2
	LR A,5
	CI 0
	BNZ SGH3
	LIS 1
	DCI VCEND
	ST
SGH2:	CLR
	LR 5,A
	LR 6,A
SGH3:	DCI VCRNK
	LR A,5
	ST
	LR A,6
	ST			; VCSUI is VCRNK+1, so the stores chain

	PI SGCARD
	DCI VCX
	LM
	AI CDPIT
	DCI VCX
	ST
	DCI VCI
	LM
	INC
	DCI VCI
	ST
	BR SGH1

SGHX:	; VCX has reached 39 + 55 = 94: the felt column past the last card
	LIS 1
	LR 0,A
	DCI VCX
	LM
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	LI CVAL3
	LR 3,A
	LIS DCELLH
	LR 9,A
	PI DFILL
	LEAVE

; ---------------------------------------------------------------------------
; SGCARD -- one 11x6 slot at VCX from the pair in VCRNK/VCSUI.
;   The arguments live in RAM because DFILL and DSTAMP between them write
;   r0 and r4-r9, and PI clobbers the accumulator on the way in.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
SGCARD:	ENTER
	LIS 1			; the felt separator, full row height
	LR 0,A
	DCI VCX
	LM
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	LI CVAL3
	LR 3,A
	LIS DCELLH
	LR 9,A
	PI DFILL

	DCI VCRNK
	LM
	CI 0
	BNZ SGC1

	LI CDW			; no card here: bare felt, face and gap row at once
	LR 0,A
	DCI VCX
	LM
	INC
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	LI CVAL3
	LR 3,A
	LIS DCELLH
	LR 9,A
	PI DFILL
	LEAVE

SGC1:	LI CDW			; the face itself
	LR 0,A
	DCI VCX
	LM
	INC
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	LI CVAL0
	LR 3,A
	LIS CDH
	LR 9,A
	PI DFILL

	LI CDW			; and the felt row under it
	LR 0,A
	DCI VCX
	LM
	INC
	LR 1,A
	DCI VCY
	LM
	AI CDH
	LR 2,A
	LI CVAL3
	LR 3,A
	LIS 1
	LR 9,A
	PI DFILL

	DCI VCRNK
	LM
	CI '?'
	BNZ SGC2

	DCI VCX			; a hole card: the lattice, both halves
	LM
	INC
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	LI CVAL2
	LR 3,A
	DCI CBAKL
	PI DSTAMP
	DCI VCX
	LM
	AI CDRW+1
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	LI CVAL2
	LR 3,A
	DCI CBAKR
	PI DSTAMP
	LEAVE

	; The colour has to be parked in RAM and the pip looked up twice: DCI is
	; the only way to reach RAM and it destroys DC0, so the art pointer and
	; the colour cannot both be held. The second walk is twenty instructions
	; against a slot that has just written 66 pixels.
SGC2:	PI SGSUIT		; -> VCCOL; its DC0 result is not used here
	DCI VCX
	LM
	INC
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	DCI VCCOL
	LM
	LR 3,A
	PI SGRNK		; DC0 = the rank's art; leaves r1, r2, r3
	PI DSTAMP

	DCI VCX
	LM
	AI CDRW+1
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	DCI VCCOL
	LM
	LR 3,A
	PI SGSUIT		; DC0 = the pip's art; leaves r1, r2, r3
	PI DSTAMP
	LEAVE

; ---------------------------------------------------------------------------
; SGSUIT -- VCCOL = the suit's colour; returns with DC0 on its 5-byte pip.
; SGRNK  -- returns with DC0 on the rank's 5-byte art.
;
; Both walk a table whose last row has a key of 0, which matches anything, so
; a corrupt byte lands on the blank entry instead of running off the end.
; CI does not write the accumulator, so the XS that follows still sees the key.
;   Leaves. Clobber A, r4, r5, DC0. Leave r1, r2, r3.
; ---------------------------------------------------------------------------
SGSUIT:	DCI VCSUI
	LM
	LR 4,A
	DCI SUITTB
SGSU1:	LM			; the key
	CI 0
	BZ SGSU2
	XS 4
	BZ SGSU2
	LM
	LM			; step over the offset and the colour
	BR SGSU1
SGSU2:	LM
	LR 5,A			; offset into CPIPS
	LM
	DCI VCCOL
	ST			; the colour
	DCI CPIPS
	LR A,5
	ADC
	POP

SGRNK:	DCI VCRNK
	LM
	LR 4,A
	DCI RNKTB
SGRN1:	LM
	CI 0
	BZ SGRN2
	XS 4
	BZ SGRN2
	LM			; step over the offset
	BR SGRN1
SGRN2:	LM
	LR 5,A
	DCI CRANKS
	LR A,5
	ADC
	POP

	INCLUDE "cardart.inc"
