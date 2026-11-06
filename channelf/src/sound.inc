; ---------------------------------------------------------------------------
; sound.inc -- the console's three notes.
;
; Sound is two bits at the top of port 5, the same port that carries the VRAM
; row. That is why every draw routine in fujidisp.inc read-modify-writes port 5
; instead of storing a row outright: a bare OUTS 5 mid-frame would cut whatever
; is ringing. This file is the mirror image -- it keeps the row bits and writes
; only the top two.
;
;   mode 0  silence      mode 1  1000 Hz      mode 2  500 Hz      mode 3  buzz
;
; There is no volume, no duration and no envelope control. The hardware rings
; the tone and an RC decay kills it with a half-life of about 9 ms, so every
; cue is a blip whatever you do. Two consequences shape all of this:
;
;   * A tone does not have to be switched off and nothing has to wait for it.
;     SNDCUE triggers and returns; the decay does the rest. That is what makes
;     a key click affordable inside INSCAN, which runs every 8 ms.
;   * The hardware retriggers only when the mode CHANGES. Blip the same tone
;     twice in a row without passing through silence and the second one is
;     inaudible -- so SNDCUE always writes 0 first.
;
; SNDCUE leaves the tone bits set on the way out, which is harmless: by the
; time anything looks again it has decayed to nothing, and the next cue starts
; with silence regardless.
; ---------------------------------------------------------------------------

SND1	EQU 040H		; 1000 Hz, the bright one
SND2	EQU 080H		; 500 Hz
SND3	EQU 0C0H		; low buzz

; SNDCUE -- blip the tone in r0, which is already in bits 7-6.
;   Leaf. Clobbers A, r8. Leaves the VRAM row in bits 5-0 alone.
SNDCUE:	INS 5
	NI 3FH
	LR 8,A			; the row bits, which are not ours to touch
	OUTS 5			; through silence: the tone retriggers on a change
	LR A,8
	AS 0			; the two fields are disjoint, so add is or
	OUTS 5
	POP

; SNDKEY -- a key click. Tail call, so it returns to SNDCUE's caller's caller:
;   JMP leaves PC1 alone. Leaf. Clobbers A, r0, r8.
SNDKEY:	LI SND1
	LR 0,A
	JMP SNDCUE

; SNDDEAL -- a card hits the table. Leaf. Clobbers A, r0, r8.
SNDDEAL: LI SND3
	LR 0,A
	JMP SNDCUE

; SNDGAP -- the pause between two notes of a cue: long enough to clear the
;   9 ms half-life so the notes are separate, short enough that they are heard
;   as one cue rather than as separate beeps.
;
;   These are DELAYMS units, which are NOT milliseconds -- measured against a
;   recording off the emulator, one unit is about 9 ms, so a cue's notes land
;   roughly 70 ms apart. DELAYMS says on the tin that it is approximate; it is
;   calibrated by what each caller wanted, and the poll loop wanted something
;   else entirely.
SNDGAP	EQU 8

; SNDSEL -- a move has been sent: two notes going up.
;   Reached by PI; calls one level down.
SNDSEL:	ENTER
	LI SND2
	LR 0,A
	PI SNDCUE
	LI SNDGAP
	LR 3,A
	PI DELAYMS
	LI SND1
	LR 0,A
	PI SNDCUE
	LEAVE

; SNDTRN -- your turn. The one cue that has to carry from across the room, so
;   it is three notes rising rather than two.
;   Reached by PI; calls one level down.
SNDTRN:	ENTER
	LI SND3
	LR 0,A
	PI SNDCUE
	LI SNDGAP
	LR 3,A
	PI DELAYMS
	LI SND2
	LR 0,A
	PI SNDCUE
	LI SNDGAP
	LR 3,A
	PI DELAYMS
	LI SND1
	LR 0,A
	PI SNDCUE
	LEAVE

; SNDEND -- the hand is over: three notes coming down, the mirror of SNDTRN so
;   the two are never mistaken for each other.
;   Reached by PI; calls one level down.
SNDEND:	ENTER
	LI SND1
	LR 0,A
	PI SNDCUE
	LI SNDGAP
	LR 3,A
	PI DELAYMS
	LI SND2
	LR 0,A
	PI SNDCUE
	LI SNDGAP
	LR 3,A
	PI DELAYMS
	LI SND3
	LR 0,A
	PI SNDCUE
	LEAVE
