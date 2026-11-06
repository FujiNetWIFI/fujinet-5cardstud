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

SGFTY	EQU DORGY+8*DCELLH	; the footer's pixel row
SGSCDIV	EQU 3			; SGWAIT2 ticks between marquee steps: a tick is about
				; 70 ms, so this is a character every fifth of a second
SGHOLD	EQU 2			; polls the end-of-hand banner stays up

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

SGDRAW:	PI SGCUES		; before the draw: it owns the round edge, and the
				; footer needs the marquee reset that comes with it
	PI SGTITLE
	PI SGROWS
	PI SGFOOT
	PI SGTURN		; and after: the your-turn cue follows the picture

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
	PI SGSCRL		; step the result marquee, if one is running
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
	BNZ SGK1A
	JMP SGMVUP
SGK1A:	CI EV_RIGHT
	BNZ SGK1B
	JMP SGMVDN
SGK1B:	CI EV_LEFT
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
SGSND1:	PI SNDSEL
	PI SGMVCODE		; VMOVE = the two-character code
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
	; cap the top of seat 0's cards with felt, in the title row's own blank
	; pixel row -- the separators then run unbroken from here to the footer
	LI CDBEDW
	LR 0,A
	LI CDX0
	LR 1,A
	LI DORGY+DCELLH-1
	LR 2,A
	LI CVAL3
	LR 3,A
	LIS 1
	LR 9,A
	PI DFILL
	LEAVE

; ---------------------------------------------------------------------------
; SGCUES -- the round edge. Resets the result marquee, because the round edge
; is exactly when the message it scrolls has changed, and queues the cue for
; SGTURN to play once the picture is up.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
SGCUES:	ENTER
	; age the result banner -- see below for why it is held rather than shown
	; only while the server happens to be in the showdown
	DCI VLRHOLD
	LM
	CI 0
	BZ SGCU0
	AI 0FFH
	DCI VLRHOLD
	ST

SGCU0:	DCI FNREPLY+GROUND
	LM
	LR 5,A			; the round now
	DCI VPROUND
	LM
	LR 6,A			; the round last time
	LR A,5
	XS 6
	BZ SGCUX		; the round did not move; the message still might have
	LR A,5
	DCI VPROUND
	ST

	; The round edge means a card hit the table.
	LIS 1
	DCI VCUEQ
	ST

	; A hand ending is a different signal, and the round cannot carry it. At
	; a small table where everyone folds, the server finishes the hand and
	; deals the next one without the round ever leaving 1 -- there is no edge
	; to see. Even at a full table the showdown can fall between two polls,
	; which are four seconds apart. What always changes is lastResult itself,
	; so that is what is watched: a checksum over it, because the winner's
	; name is often the same from hand to hand and only the hand description
	; differs. A checksum can collide, in which case one banner is missed;
	; comparing 80 bytes every poll to avoid that is not worth it.
SGCUX:	DCI FNREPLY+GLAST
	CLR
	LR 4,A			; running sum
	LI 80
	LR 5,A			; lastResult is 81 bytes and need not be terminated
SGCK1:	LM
	CI 0
	BZ SGCK2
	AS 4
	LR 4,A
	DS 5
	BNZ SGCK1
SGCK2:	DCI VLRSUM
	LM
	XS 4
	BZ SGCKX		; the same message we already had
	LR A,4
	DCI VLRSUM
	ST

	LI SGHOLD
	DCI VLRHOLD
	ST
	CLR
	DCI VLRPOS
	ST			; a new message: start the marquee over
	LI SGSCDIV
	DCI VLRTIK
	ST
	LIS 2
	DCI VCUEQ
	ST			; the hand-over cue outranks the deal cue
SGCKX:	LEAVE

; ---------------------------------------------------------------------------
; SGTURN -- play what SGCUES queued, then the your-turn cue. Runs AFTER the
;   draw, so a cue never delays the picture it belongs to.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
SGTURN:	ENTER
	; whatever SGCUES queued, now that the picture is up
	DCI VCUEQ
	LM
	LR 5,A
	CLR
	DCI VCUEQ
	ST
	LR A,5
	CI 1
	BNZ SGTU1
	PI SNDDEAL
	BR SGTU2
SGTU1:	CI 2
	BNZ SGTU2
	PI SNDEND

	; The your-turn cue, on the EDGE where the server starts offering moves:
	; /state is polled about once a second and it stays your turn for many
	; polls, so on the level it would machine-gun.
SGTU2:	DCI FNREPLY+GNMOVE
	LM
	LR 5,A
	DCI VPNMV
	LM
	LR 6,A
	LR A,5
	DCI VPNMV
	ST
	LR A,5
	CI 0
	BZ SGTUX		; no moves now
	LR A,6
	CI 0
	BNZ SGTUX		; there were moves last time too: not an edge
	PI SNDTRN
SGTUX:	LEAVE

; ---------------------------------------------------------------------------
; SGSCRL -- step the end-of-hand marquee one character.
;
; The message runs to about 45 characters -- "Hulk BOT won with Pair, Kings" is
; 29 -- against a line 23 cells wide, so it is scrolled rather than cut: the
; interesting half is the end. VLRON says a result is on screen at all, and
; SGFOOT stops advancing once the tail is visible, so it scrolls once and then
; holds rather than looping forever under the showdown.
;   Reached by PI; calls one level down.
; ---------------------------------------------------------------------------
SGSCRL:	ENTER
	DCI VLRON
	LM
	CI 0
	BZ SGSCX
	DCI VLRTIK
	LM
	AI 0FFH
	DCI VLRTIK
	ST
	CI 0
	BNZ SGSCX
	LI SGSCDIV
	DCI VLRTIK
	ST

	; one more character, unless the end of the message is already on screen
	DCI VLRPOS
	LM
	AI UICOLS
	LR 5,A
	DCI VLRLEN
	LM
	COM
	INC
	AS 5			; (pos + UICOLS) - len
	BM SGSC1
	LEAVE			; the tail is showing: hold here
SGSC1:	DCI VLRPOS
	LM
	INC
	DCI VLRPOS
	ST
	PI SGFOOT
SGSCX:	LEAVE

; SGFOOT -- every legal move on one line, the selected one in the accent
;   colour, so you can see what else you could do and not only what is about to
;   be sent. Names run left to right a space apart and the last one is clipped
;   at the edge of the line rather than wrapping: the server's longest name is
;   "Raise 5" and it offers at most three at once, so the clip is a guard, not
;   the common case.
;
;   The cell counters here are all under 24, so the signed compares are safe --
;   the carry-not-sign rule bites only once a value can pass 127.
;   Reached by PI; calls one level down.
SGFOOT:	ENTER
	LI UICOLS
	LR 0,A
	LI DORGX
	LR 1,A
	LI SGFTY
	LR 2,A
	PI DCLRR

	DCI FNREPLY+GNMOVE
	LM
	CI 0
	BZ SGF0
	JMP SGF1		; the result branch below outgrew a relative branch

	; No moves offered. The server leaves lastResult holding the PREVIOUS
	; hand's winner for the whole of the next hand, so showing it whenever
	; the menu is down would park a stale banner on screen all hand long. It
	; is the news only just after a hand has ended.
SGF0:	CLR
	DCI VLRON
	ST
	DCI VLRHOLD
	LM
	CI 0
	BNZ SGFRES		; a hand ended in the last poll or two
	DCI FNREPLY+GROUND
	LM
	CI 0
	BZ SGFRES		; between hands
	CI 4
	BM SGFRES		; CI is (immediate - A), so BM here means round > 4

	LI CVAL1
	LR 3,A
	LI DORGX
	LR 1,A
	LI SGFTY
	LR 2,A
	DCI SGTHEIRS
	PI DSTR
	LEAVE

	; Measure the message first: SGSCRL needs to know when the tail has come
	; into view so it can stop rather than scroll off the end.
SGFRES:	DCI FNREPLY+GLAST
	CLR
	LR 4,A
SGFR1:	LM
	CI 0
	BZ SGFR2
	LR A,4
	INC
	LR 4,A
	CI 80
	BZ SGFR2		; lastResult is 81 bytes and need not be terminated
	BR SGFR1
SGFR2:	LR A,4
	DCI VLRLEN
	ST
	CI 0
	BZ SGFRX		; nothing to say
	LIS 1
	DCI VLRON
	ST

	DCI VLRPOS
	LM
	LR 5,A
	DCI FNREPLY+GLAST
	LR A,5
	ADC			; the window starts VLRPOS into the message
	XDC
	DCI VMVBUF
	XDC			; DC0 = the message, DC1 = the buffer
	LI UICOLS
	LR 4,A
SGFR3:	LR A,4
	CI 0
	BZ SGFR4
	LM
	CI 0
	BZ SGFR4
	CI 60H			; 'a'-1: fold to upper case, as the move names are
	BP SGFR3S
	CI 7AH
	BM SGFR3S
	AI 0E0H
SGFR3S:	XDC
	ST
	XDC
	DS 4
	BR SGFR3
SGFR4:	XDC
	CLR
	ST
	XDC
	LI CVAL2
	LR 3,A
	LI DORGX
	LR 1,A
	LI SGFTY
	LR 2,A
	DCI VMVBUF
	PI DSTR
SGFRX:	LEAVE

	; The list changes every turn -- three moves this time, two the next -- so a
	; selection carried over from the last one can point past the end, and
	; SGMVCODE would then copy whatever follows the table.
SGF1:	CLR
	DCI VLRON
	ST
	DCI VMVSEL
	LM
	LR 5,A
	DCI FNREPLY+GNMOVE
	LM
	LR 4,A
	LR A,5
	COM
	INC
	AS 4			; count - sel
	BZ SGF1R
	BM SGF1R
	BR SGF1K
SGF1R:	CLR
	DCI VMVSEL
	ST

SGF1K:	CLR
	DCI VMVI
	ST
	CLR
	DCI VMVCOL
	ST

SGF2:	DCI VMVI
	LM
	LR 4,A
	DCI FNREPLY+GNMOVE
	LM
	XS 4
	BNZ SGF2A

	; The loop body is well past the +/-127 a relative branch reaches, so the
	; three ways out of it share one trampoline.
SGFXJ:	JMP SGFX		; every move drawn, or the line is full

SGF2A:	DCI VMVCOL
	LM
	LR 6,A
	CI UICOLS
	BZ SGFXJ
	BM SGFXJ
	LR A,6
	COM
	INC
	AI UICOLS
	LR 6,A			; cells left -- in r6, which SGMVOFF does not eat

	PI SGMVOFF
	LIS 3
	ADC			; step over code[3] to the name
	XDC
	DCI VMVBUF
	XDC			; DC0 = the name, DC1 = the buffer
	LR A,6
	LR 4,A			; characters still allowed
	CLR
	LR 5,A			; characters copied

SGF3:	LR A,4
	CI 0
	BZ SGF4			; no room left: clip the name here
	LM
	CI 0
	BZ SGF4			; end of the name
	; The wire sends the names lower case -- "fold", "raise 5" -- and this
	; font's lower case is the uppercase glyph squashed into four rows, so
	; they would sit a row short of everything else while the digit in
	; "raise 5" stood full height next to them. Fold to upper case. CI
	; computes (immediate - A), so BP means A <= the immediate.
	CI 60H			; 'a'-1
	BP SGF3S
	CI 7AH			; 'z'
	BM SGF3S
	AI 0E0H			; -20H
SGF3S:	XDC
	ST
	XDC
	LR A,5
	INC
	LR 5,A
	DS 4
	BR SGF3

SGF4:	XDC
	CLR
	ST
	XDC
	LR A,5
	DCI VMVLEN
	ST			; DSTR eats r4-r8, so the length goes to RAM

	LI CVAL3
	LR 3,A
	DCI VMVI
	LM
	LR 6,A
	DCI VMVSEL
	LM
	XS 6
	BNZ SGF5
	LI CVAL2
	LR 3,A			; the one FIRE will send

SGF5:	DCI VMVCOL
	LM
	SL 1
	SL 1
	AI DORGX
	LR 1,A
	LI SGFTY
	LR 2,A
	DCI VMVBUF
	PI DSTR

	DCI VMVLEN
	LM
	INC
	LR 4,A			; the name plus one space
	DCI VMVCOL
	LM
	AS 4
	DCI VMVCOL
	ST
	DCI VMVI
	LM
	INC
	DCI VMVI
	ST
	JMP SGF2
SGFX:	LEAVE

; SGMVOFF -- DC0 = FNREPLY + GMOVES + VMVI*GMVSTR. The index is VMVI rather
;   than VMVSEL so SGFOOT can walk the whole list with it.
;   Leaf. Clobbers A, r4.
SGMVOFF: DCI VMVI
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

; SGMVCODE -- copy the selected move's code into VMOVE.
;
;   The codes are two characters in a 3-byte field -- CA, FO, RA, CH, BB, BL --
;   and BLDURL builds /move/<code>?table=... from VMOVE. An empty VMOVE
;   therefore asks for /move/? , which the server answers 404 and acts on not
;   at all: the next /state still says it is your turn. That is what "I chose
;   call and it kept asking me to input a move" was.
;
;   The copy below used to XDC BEFORE the LM, which reads the destination and
;   writes the source -- so it copied VMOVE over the reply window (which the
;   cart will not let the console write anyway) and left VMOVE empty. Read the
;   source first, exactly as SGPLINE does.
;   Reached by PI; calls one level down.
SGMVCODE: ENTER
	DCI VMVSEL
	LM
	DCI VMVI
	ST
	PI SGMVOFF		; DC0 = the move record, at its code
	XDC
	DCI VMOVE
	XDC			; DC0 = the code, DC1 = VMOVE
	LIS 3
	LR 4,A
SGMC1:	LM			; the SOURCE first
	CI 0
	BZ SGMC2
	XDC
	ST
	XDC
	DS 4
	BNZ SGMC1
SGMC2:	XDC
	CLR
	ST			; terminate where the copy actually stopped
	XDC
	LEAVE

; SGROWS -- one row per player. Reached by PI; calls two levels down.
;
; The name is drawn with DSTR directly rather than through UIROW, which
; hardcodes the lobby's gutter offset: this screen has no cursor, so the row
; starts at DORGX and the four pixels UIROW would skip go to the cards.
SGROWS:	ENTER
	DCI VROW
	CLR
	ST
SGR1:	DCI VROW
	LM
	CI NPLRROW
	BNZ SGR2
	LEAVE

SGR2:	DCI VROW
	LM
	LR 0,A
	PI UIROWY		; r2 = this row's top pixel row
	LR A,2
	DCI VCY
	ST			; in RAM: DCLRR and DFILL both walk r2 down

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
	DCI VROW		; no player here: clear the row flat, cards and all
	LM
	LR 0,A
	PI UICLRR
	JMP SGRN

	; Only the text field is cleared. Every pixel of the card bed is written
	; exactly once by SGHAND, so clearing it first would be paying for it
	; twice -- and 9 cells plus the bed comes to the same 552 pixels the
	; full-width clear used to cost on its own.
SGRD:	LI SGTXTW
	LR 0,A
	LI DORGX
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	PI DCLRR

	PI SGPLINE		; VLINE = the name, VDIGIT = the purse

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

	; r3 survives DCLRR, DSTR and SGDLEN, so the colour is set once here and
	; used by both draws. The cards never take it: a suit's colour has to
	; mean the suit.
SGRD2:	LI DORGX
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	DCI VLINE
	PI DSTR

	; The purse is right-aligned so its last cell ends where the felt begins.
	; DEC5 suppresses leading zeros and emits up to five characters, so a
	; left-aligned field would either lie about a four-figure purse or push
	; the bed off the safe area; this way a rich player eats into the tail of
	; their own name instead, and the cards never move.
	PI SGDLEN		; r4 = the length
	LR A,4
	SL 1
	SL 1
	COM
	INC
	AI CDX0+1		; 40 - 4*len
	LR 1,A
	DCI VCY
	LM
	LR 2,A
	DCI VDIGIT
	PI DSTR

	PI SGHAND

SGRN:	DCI VROW
	LM
	INC
	DCI VROW
	ST
	JMP SGR1

; SGDLEN -- r4 = the number of characters in VDIGIT, one to five.
;   Leaf. Clobbers A, r4, DC0. Leaves r1, r2, r3.
SGDLEN:	DCI VDIGIT
	CLR
	LR 4,A
SGDL1:	LM
	CI 0
	BZ SGDLX
	LR A,4
	INC
	LR 4,A
	BR SGDL1
SGDLX:	POP

; SGPLINE -- VLINE = the player's name clamped to six characters, VDIGIT =
;   their purse. The cards are no longer part of the row string: SGHAND draws
;   them as tiles straight out of the record.
;   Reached by PI; calls one level down.
SGPLINE: ENTER
	; the purse first: DEC5 walks DC1, which the row builder below needs
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
	LIS 6
	LR 4,A
SGPL1:	LM
	XDC
	ST
	XDC
	CI 0
	BZ SGPLX		; ST leaves A alone, so this still sees the character
	DS 4
	BNZ SGPL1
	XDC
	CLR
	ST
	XDC			; six characters and no NUL of its own: add one
SGPLX:	LEAVE

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
