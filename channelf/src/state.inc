; ---------------------------------------------------------------------------
; state.inc -- the wire format, as offsets.
;
; The server speaks a fixed binary layout when the request carries ?bin=1,
; little-endian for the 16-bit fields (its util.go appendFixedLengthString).
; Nothing here is parsed into RAM: the screens index the reply window
; directly, which is legal because the mailbox repaints it only on a SEQ
; commit -- see the reply-stability invariant in fuji_mailbox.h.
; ---------------------------------------------------------------------------

; --- /state reply ---
GLAST	EQU 0			; lastResult[81]
GROUND	EQU 81
GPOTLO	EQU 82			; pot, u16 LE
GPOTHI	EQU 83
GACTIVE	EQU 84			; active player, $FF = none
GMOVET	EQU 85			; seconds left on the move
GVIEW	EQU 86			; which seat we are
GNMOVE	EQU 87			; how many valid moves follow
GMOVES	EQU 88			; 5 x { code[3], name[10] }
GMVSTR	EQU 13
GNPLAY	EQU 153
GPLAYER	EQU 154			; players, PSTRIDE apart

PSTRIDE	EQU 33
PNAME	EQU 0			; name[9]
PSTATUS	EQU 9			; 0 waiting, 1 playing, 2 folded, 3 left
PBETLO	EQU 10			; bet, u16 LE
PBETHI	EQU 11
PMOVE	EQU 12			; move[8]
PPURSLO	EQU 20			; purse, u16 LE
PPURSHI	EQU 21
PHAND	EQU 22			; 5 cards as 2 ASCII bytes each, then a NUL

GMINLEN	EQU 154
GMAXLEN	EQU 418			; 154 + 33*8

; --- /tables reply ---
TSTRIDE	EQU 36
TID	EQU 0			; id[9]
TNAME	EQU 9			; name[21]
TPLAY	EQU 30			; players[6], "cur / max"
TMAXLEN	EQU 361

; --- N: device ---
NDEV	EQU 071H
NCOPEN	EQU 04FH		; 'O'
NCCLOSE	EQU 043H		; 'C'
NCREAD	EQU 052H		; 'R'
NCSTAT	EQU 053H		; 'S'
NMHTTPG	EQU 00CH		; OPEN_MODE_HTTP_GET_H
NTRNONE	EQU 000H		; no EOL translation

; STATUS reply: avail u16 LE, connected, nDevStatus_t
NSAVLO	EQU 0
NSAVHI	EQU 1
NSCONN	EQU 2
NSDEVST	EQU 3

; --- appkeys, on the Fuji device: the username every client in the family
; --- shares (creator 1, app 1, key 0) ---
FC_AKOPN EQU 0DCH
FC_AKCLS EQU 0DBH
FC_AKRD	EQU 0DDH
FC_AKWR	EQU 0DEH
AKCREAT	EQU 1
AKAPP	EQU 1
AKKEY	EQU 0
