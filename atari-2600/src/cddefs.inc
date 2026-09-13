; cddefs.inc -- the wire format, the screen, and the 128 bytes.
;
; Equates only, so this goes BEFORE the ORG next to fujinet.inc.
;
; This console has 128 bytes of RAM and the stack mirrors into the top of
; them. What that buys, once accepted, is a design where the CARTRIDGE holds
; the state: the server's 418-byte reply sits in the cartridge's 512-byte
; window and is read from there IN PLACE, each seat's five cards are composed
; in the cartridge by FB_CARD, and the player name and table id live in the
; cartridge's path buffers. Nothing below is a buffer.

; ---------------- the banks ----------------
; A bank switch replaces every byte of $1000-$17FF, so each bank carries its
; own copy of every module it calls; only zero page crosses. The text planes
; and the reply window are CARTRIDGE state and survive the switch, which is
; why a bank change is a screen change and nothing has to be copied.
;
; Image = (N+1) * 2048 with N in {1,3,7,15} -- MAME's cart slot accepts no
; other size -- so seven banks and 16K.
;
; THE FETCH HAS A BANK OF ITS OWN, and that is the whole reason there are
; seven. net.inc and url.inc are 550 bytes together and the game bank needs
; every byte it can get for the display kernel, the renderer and the move
; menu; it came in 552 bytes over with them in it. A poll happens every 90
; frames, so paying a bank switch for one costs nothing, and a bank with no
; display kernel in it does not care that the screen is blanked while it
; talks -- it was blanked for the transaction anyway.
BANKLOB EQU     0               ; boot and the table list. Bank 0 is where the
                                ;   cold stub sends us, so it is the first
                                ;   screen by construction.
BANKGAM EQU     1               ; the table: display, render, bet
BANKNET EQU     2               ; one poll, and back
BANKMNU EQU     3               ; the in-game menu, leaving, help
BANKNAM EQU     4               ; the on-screen keyboard and the username
BANKSPR EQU     5               ; two spare banks. An image has to be one of
                                ;   four sizes and 16K is the one that fits,
                                ;   so these exist whether or not they are
                                ;   used; each holds a stub that returns to
                                ;   bank 0, so a stray switch is survivable
                                ;   rather than a jump into zeros.
NBANKS  EQU     7

; The trampoline, at a fixed address in the fixed tail because the banks are
; assembled separately. See cdtail.asm.
CDGOTO  EQU     FNTAIL

; ---------------- what to do on arrival ----------------
; A bank switch is a JUMP, never a call: the store that switches is the last
; instruction fetched from the old bank, so there is no return address to come
; back to. "Where to carry on" is therefore DATA, in CDENT, exactly as the
; Odyssey 2 port's X_RET is.
ENCOLD  EQU     0               ; the lobby, from scratch
ENLOBBY EQU     1               ; the lobby, redrawing what is already there
ENGCOLD EQU     2               ; the table, from scratch
ENGAME  EQU     3               ; the table, back from a poll
ENFETCH EQU     4               ; the fetch bank: one poll
ENTABLE EQU     5               ; the fetch bank: one /tables
ENMENU  EQU     6               ; the in-game menu
ENLEAVE EQU     7               ; the menu bank: send /leave, then the lobby
ENNAME  EQU     8               ; the keyboard

; ---------------- requests ----------------
RQTABLE EQU     0
RQSTATE EQU     1
RQMOVE  EQU     2
RQLEAVE EQU     3

; ---------------- the path buffers ----------------
; The two strings that must survive a request that repaints the reply window.
; They are held in the CARTRIDGE: 128 bytes of console RAM cannot spare 19 for
; them, and FP_TXRAW streams one straight into a URL without ever landing in
; console memory at all.
PBTABLE EQU     FP_SEL0         ; the joined table's id
PBNAME  EQU     FP_SEL1         ; the player name
PBEDIT  EQU     FP_SEL3         ; what the keyboard is typing into

; ---------------- the wire format, ?bin=1 ----------------
; The server speaks a fixed little-endian binary layout when the request
; carries ?bin=1 (its util.go appendFixedLengthString). Identical to the
; shared C core's _Packed struct Game in fujinet-5cardstud/src/misc.h and to
; the offsets in the intv, arcadia, astrocade, o2 and channelf ports.
;
; Do NOT ask for &be=1. The Go server's binary.BigEndian path is the road much
; less travelled, and the Intellivision port found pot and bet corruption on
; it that always landed on exact multiples of 256.
;
; /state, /move/XX and /leave all answer with a Game.
GLAST   EQU     0               ; lastResult[81], the end-of-hand banner
GROUND  EQU     81              ; 0 waiting, 1-4 streets, 5 showdown
GPOTLO  EQU     82              ; pot, u16 LE
GPOTHI  EQU     83
GACTIVE EQU     84              ; int8; $FF nobody, 0 you
GMOVET  EQU     85              ; seconds allowed, already net of a grace
                                ;   period the server computes
GVIEW   EQU     86              ; 1 = you are a spectator
GNMOVE  EQU     87              ; how many of validMoves[] are live
GMOVES  EQU     88              ; 5 slots ALWAYS on the wire, MVSTRID apart
GNPLAY  EQU     153             ; playerCount
GPLAYER EQU     154             ; playerCount records, PLSTRID apart

MVSTRID EQU     13
MVCODE  EQU     0               ; move[3]: a 2-character code and a NUL
MVNAME  EQU     3               ; name[10]; the longest the server sends is
                                ;   "ALL-IN", six characters
MAXMOVE EQU     5

PLSTRID EQU     33
PLNAME  EQU     0               ; name[9]
PLSTAT  EQU     9               ; PS* below
PLBETLO EQU     10              ; bet, u16 LE
PLBETHI EQU     11
PLMOVE  EQU     12              ; move[8]: the seat's last action, as text
PLPRSLO EQU     20              ; purse, u16 LE
PLPRSHI EQU     21
PLHAND  EQU     22              ; hand[11]: five cards as two lowercase ASCII
                                ;   bytes each, then a NUL. "??" is hidden.
MAXPLYR EQU     8

; Seat status.
PSWAIT  EQU     0
PSPLAY  EQU     1
PSFOLD  EQU     2
PSLEFT  EQU     3

GMINLEN EQU     154             ; the fixed prefix, nobody seated
GMAXLEN EQU     418             ; 154 + 33*8 -- inside slice 0 whole, which is
                                ;   why the reply window is 512 and not 256

; /tables
TBSTRID EQU     36
TBID    EQU     0               ; id[9]
TBNAME  EQU     9               ; name[21]
TBPLYRS EQU     30              ; players[6], the literal string "cur / max"
TMAXLEN EQU     361             ; 1 + 10*36
MAXTBL  EQU     10

; ---------------- the screen: 12 columns, 21 rows ----------------
;   0        P<pot:5>$<purse:5>          the pot, and YOUR purse
;   1-16     eight seats, two rows each:
;              upper   five card ranks (pixels 2-30) | name, columns 8-11
;              lower   five suit pips  (pixels 2-30) | bet or purse, 8-11
;   17-20    the bottom bar: the move menu with the red bar on the selection,
;            or WAITING ON <name>, or a page of the end-of-hand banner
;
; Eight seats at two rows each is sixteen of the twenty-one, and that is the
; whole budget: one row of chrome above and four below. A seat's status is
; carried by its COLOUR rather than by a character, which is what pays for the
; four-character name and the four-digit value beside every hand.
RHEAD   EQU     0
RSEAT0  EQU     1
RSEATH  EQU     2               ; rows per seat
RBAR0   EQU     17
NBARROW EQU     4

; ---------------- colours (NTSC) ----------------
CGREEN  EQU     $C4             ; the felt: the frame and the top/bottom bands
CBLACK  EQU     $00             ; the panel
CDARK   EQU     $02             ; alternate seat bands, so eight rows separate
CRED    EQU     $44             ; the move bar
CWHITE  EQU     $0E             ; chrome, and your own seat
CGREY   EQU     $0A             ; the other seats
CDIM    EQU     $04             ; folded or departed
CGOLD   EQU     $1E             ; the seat to act

; ---------------- zero page ----------------
; $80-$8C is the transport's (fujinet.inc) and $81/$82 the display kernel's;
; $A8/$A9 are INSCAN's. Everything else below is ours, and the stack must stay
; above $C0.
;
; $8D-$A7 is the ROM swap stub's landing ground in the firmware's own clients.
; This program never swaps -- it boots nothing -- so it is ours.
CDIDX   EQU     $84             ; a render loop's index. Its own cell because
                                ;   CDTMP is not safe across a call: CDDEC
                                ;   used to borrow CDTMP for its field width,
                                ;   and the seat loop that kept its index
                                ;   there looped forever and never unblanked
                                ;   the screen.
CDTMP   EQU     $85             ; scratch. SHORT-LIVED means exactly that: any
                                ;   routine you call may clobber it.
INRPT   EQU     $8D             ; INREPT's auto-repeat countdown
CDDGT   EQU     $8E             ; CDDEC's digit in bits 0-3, and its
                                ;   leading-zero latch in bit 7
CDREQ   EQU     $8F             ; the request in flight (RQ*)
CDTRIES EQU     $90             ; the STATUS settle loop's bound
CDDLY   EQU     $91             ; its inter-poll delay: in memory, because
                                ;   CDWAIT counts in X and Y
; The input mask for this frame, in a cell of its own.
;
; It CANNOT live in CDTMP. Every draw routine borrows CDTMP -- the move menu's
; slot arithmetic, the banner's line length, the keyboard's row offset -- and
; the input tests are interleaved with draw calls, so the second test of a
; frame was reading a move index or a string length as a button press. That
; walked the move cursor on its own, and in the in-game menu it pressed LEAVE.
;
; It borrows the settle loop's cell instead. Nothing overlaps: the bank that
; polls STATUS has no APPVBL in it, and the banks with an APPVBL never poll.
CDINP   EQU     $92

CDAVL0  EQU     $92             ; NET_STATUS bytes waiting
CDAVL1  EQU     $93
CDPRV0  EQU     $94             ; the previous reading, for the settle loop
CDPRV1  EQU     $95

; CDDEC's 16-bit accumulator shares the settle loop's cells. The two provably
; cannot be live together: CDPRV0/1 exist only inside APICALL's STATUS poll,
; and nothing composes a screen during a transaction -- APPVBL runs the
; transaction to completion and only then draws. Every byte of $80-$BF is
; spoken for, and this is the one overlap with an argument behind it.
CDNUM0  EQU     CDPRV0          ; the 16-bit value CDDEC is printing
CDNUM1  EQU     CDPRV1
CDRXL0  EQU     $96             ; the READ's reply length, captured at once
CDRXL1  EQU     $97
CDMIN0  EQU     $98             ; the minimum this request will settle for
CDMIN1  EQU     $99
CDERR   EQU     $9A             ; the last failure
CDSTEP  EQU     $9B             ; where it happened

; The display kernel's. CDSCOL is eight bytes and the kernel indexes it on the
; seam line, which is why the seat colours are decided once per render rather
; than recomputed per scanline out of the reply window.
CDROW   EQU     $9C             ; the row being drawn, 0-20
CDLINE  EQU     $9D             ; ink lines left in it
CDBAR   EQU     $9E             ; the row the red bar is on, or $FF
CDSCOL  EQU     $9F             ; eight seat colours, $9F-$A6

; APICALL's return value, parked while CDMOVE is cleared. It borrows the
; kernel's row counter, which is safe because the bank that issues requests
; has no kernel in it -- the same bank-local overlap CDCNT and SNDCNT use.
CDERR2  EQU     CDROW

CDPOLL  EQU     $A7             ; frames until the next poll
CDSEL    EQU    $AA             ; list cursor, or the selected move
CDCNT    EQU    $AB             ; rows in the list -- LOBBY BANK ONLY
SNDCNT   EQU    CDCNT           ; frames left on a sound cue -- GAME BANK
                                ;   ONLY. The two banks are different 2K
                                ;   images and only zero page crosses between
                                ;   them, so a cell each bank alone uses is
                                ;   one cell. Battleship overlaps its
                                ;   placement screen with its play bank the
                                ;   same way.
CDNPLR   EQU    $AC             ; playerCount from the last reply
CDROUND  EQU    $AD             ; round
CDACT    EQU    $AE             ; activePlayer, $FF for nobody
CDNMV    EQU    $AF             ; validMoveCount
CDMVTOP  EQU    $B0             ; the first move row shown, when there are
                                ;   five and only four rows to show them in
CDCLK    EQU    $B1             ; the local move countdown, in seconds
CDTICK   EQU    $B2             ; frames until it ticks
CDLRSUM  EQU    $B3             ; the lastResult checksum
CDLRPG   EQU    $B4             ; banner page, 0 = not showing
CDLRHLD  EQU    $B5             ; frames this page is held
CDPRVAC  EQU    $B6             ; the previous activePlayer, for the turn edge
CDMHELP  EQU    CDSELHD         ; the help screen is up -- MENU BANK ONLY.
                                ;   The purse overlay is GAME BANK ONLY.
CDSELHD  EQU    $B7             ; SELECT held, so the value fields repaint on
                                ;   the change and not every frame
CDMOVE   EQU    $B8             ; the staged move code, 2 bytes and a NUL
CDENT    EQU    $BB             ; EN*: what the bank being entered should do
CDFRAME  EQU    $BC             ; a free-running frame counter
CDVIEW   EQU    $BD             ; the spectator flag, after the server hack
CDREDRW  EQU    $BE             ; nonzero: clear every row before composing
CDPRVRD  EQU    $BF             ; the previous round, for the deal cue
CDNLEN   EQU    CDPRVRD         ; characters typed so far -- NAME BANK ONLY.
                                ;   The deal cue is GAME BANK ONLY, and the
                                ;   two banks are different 2K images.

; How long the banner holds a page, and how long a poll waits. The move clock
; is WALL CLOCK on the server, so these are real seconds and a run that is not
; throttled sits at "starting in 2" forever.
POLLFRM EQU     90              ; ~1.5s between /state polls
FAILFRM EQU     150             ; ~2.5s after a failure
LRHOLD  EQU     90              ; ~1.5s a banner page
