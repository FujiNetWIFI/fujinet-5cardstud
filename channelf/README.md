# 5 Card Stud for the Fairchild Channel F

A standalone F8 client, in the Arcadia and Odyssey&sup2; mould: the shared C
core cannot fit these machines, so the game is written to them.

    make            # -> build/5card.bin, 16384 bytes

## What is different here

RAM is not the constraint. The Arcadia client runs in 84 bytes and the
Odyssey&sup2; client in 39, because neither cartridge edge can accept a write
cycle; this one can, so the cart hands the console 30K. The screens still
render straight out of the mailbox's reply window rather than buffering the
wire format -- not to save RAM, but because it is simpler and the
reply-stability invariant makes it free.

## Screen

23 columns x 9 rows of 4x6 cells: a title, seven seats, a footer. A seat row is

    NAME__ ppp | card | card | card | card | card

six of name, then the purse, then five cards as 10x5 tiles on an 11-pixel pitch
from x=39. There is no cursor gutter on this screen -- only the lobby has a
cursor -- so the row starts at x=4 and those four pixels go to the cards. The
player to act has their name and purse in the accent colour; the cards never
take it, because a suit's colour has to mean the suit.

### Why the felt is drawn and the card is not

The palette is one choice for the whole screen and there is no white in any of
them: value 0 is LTGRAY and values 1-3 are BLUE, RED, GREEN. The lightest thing
on the machine is therefore the background itself, so a card face is not
painted -- it is what is left when the felt is painted around it. Hearts and
diamonds draw red, spades and clubs blue, which is as close to black as this
console gets.

A face is 10 pixels: a 5-wide rank then a 5x5 suit pip, both stamped from
`cardart.inc`. The ranks are the font's own uppercase glyphs, moved into the
middle of a 5-wide field by the generator rather than at +1 at runtime -- which
is what lets the ten be an ordinary table entry drawing "10" instead of a
special case drawing "T", and means the wire's lowercase `t` never reaches the
font, whose lowercase forms are squashed four-row variants. A hole card is a
red lattice across the whole face; an undealt slot is bare felt, so how many
cards are out reads at a glance.

### The redraw is free

Cards cost nothing per poll, which is not obvious and did not have to be true.
**No pixel is written twice.** `SGROWS` clears only the nine text cells, and
`SGCARD` writes each slot's separator, face and gap row disjointly: 216 + 336
is exactly the 552 pixels the old full-width clear cost on its own. Filling the
whole bed with felt and punching the faces back out -- the obvious way to do it
-- would have cost 45% more on a screen that already visibly repaints.

Spade and club are the hard pair at 5x5. They are separated at silhouette
level, not by one interior pixel: the spade widens downward to a solid row and
necks into a 1px stem, the club narrows downward from a notched shoulder. One
pixel of difference would not survive composite video.

### The footer

Every legal move is on the footer line at once, left to right a space apart,
with the selected one in the accent colour -- stick left/right or up/down moves
between them, FIRE sends. The server's longest name is "Raise 5" and it offers
at most three at a time, so the worst real case is 17 of the 23 cells; a name
that would run off the end is clipped rather than wrapped, and a move that
cannot start is dropped.

The names arrive lower case (`fold`, `raise 5`) and are folded to upper case on
the way out. This font's lower case is the uppercase glyph squashed into four
rows, so left alone the words sat a row short of everything else on screen
while the digit in "raise 5" stood full height right next to them.

The selection is also clamped against `validMoveCount` on every repaint: the
list changes every turn -- three moves this time, two the next -- and a
selection carried over from the last one would otherwise index past the table.

### Sound

Sound is two bits at the top of port 5 -- the same port that carries the VRAM
row, which is why every draw routine read-modify-writes it rather than storing
a row outright. Three tones, no volume, no duration: the hardware rings the
tone and an RC decay kills it with a half-life of about 9 ms, so every cue is a
blip whatever you do.

Two things follow, and both are load-bearing. A tone never has to be switched
off and nothing has to wait for it, which is what makes a key click affordable
inside `INSCAN` -- a leaf that runs every 8 ms in a dozen polling loops, and
which now funnels its ten exits through one `INOUT` so the click costs one copy
of itself. And the hardware retriggers only when the mode CHANGES, so `SNDCUE`
writes silence before every tone or the second of two identical blips is
inaudible.

    click     1000 Hz          every accepted keypress, on every screen
    deal      low buzz         the round advanced
    your turn 3 notes rising   the server is offering you moves
    sent      2 notes rising   a move went out
    hand over 3 notes falling  the mirror of your-turn, so the two never blur

Cues are queued by `SGCUES` and played by `SGTURN` *after* the draw: a cue is
the best part of a fifth of a second of blocking delay, and played on the spot
it would run before the cards it is announcing were on screen. The your-turn
cue fires on the EDGE where moves start being offered -- on the level it would
machine-gun, since it stays your turn for many polls.

`SNDGAP` is in `DELAYMS` units, which are not milliseconds. Measured off a
recording from the emulator, one unit is about 9 ms. `DELAYMS` says on the tin
that it is approximate; it was calibrated by what the poll loop wanted.

### The end-of-hand banner

When no moves are offered the footer carries the result of the last hand,
folded to upper case and scrolled: "Hulk BOT won with Two Pair, Aces over
Kings" is 43 characters against a line 23 cells wide, and the interesting half
is the end. It scrolls a character every fifth of a second and then holds once
the tail is in view, rather than looping.

**What triggers it is the message changing, not the round.** The obvious
trigger -- round 5, the showdown -- is wrong twice over. At a small table where
everyone folds, the server finishes the hand and deals the next one without the
round ever leaving 1, so there is no edge to see at all; and even at a full
table the showdown can fall between two polls, which are four seconds apart.
Watching `lastResult` itself catches both. It is compared by checksum, because
the winner's name is often the same from hand to hand and only the description
differs; a collision costs one missed banner, which is not worth 80 bytes of
comparison every poll.

The banner is then held for a couple of polls rather than shown only while the
server happens to be in the end state. The rest of the time the footer says
WAITING -- the server leaves `lastResult` holding the previous hand's winner
for the whole of the next hand, so showing it whenever the menu is down would
park a stale banner on screen all hand long.

### The purse is right-aligned

`DEC5` suppresses leading zeros and emits one to five characters. A
left-aligned field would either lie about a four-figure purse or push the bed
off the safe area, so the purse is drawn right-aligned to end where the felt
begins: a rich player eats into the tail of their own name, and the cards never
move.

## The five transport rules

Carried verbatim from the Odyssey&sup2;, Astrocade and Arcadia ports, every one
of them learned the hard way:

1. **The CLOSE belongs at the start of the next request**, never after the
   READ. Every transaction repaints the whole reply window, a CLOSE reply is
   empty, and every screen renders out of that window between polls.
2. **Capture the reply length immediately after the READ** -- it belongs to the
   most recent transaction.
3. **Poll STATUS until two consecutive readings agree**, bounded, rather than
   guessing a delay.
4. **The 4th STATUS byte is `nDevStatus_t` and checking it is not optional**:
   the HTTP GET is deferred until the first STATUS, and an HTTP error page has
   a perfectly readable body.
5. **Validate the reply length against what `playerCount` implies** before
   believing a byte of it.

Rule 3 needed a sixth clause here. Two agreeing readings are not enough on
their own: a `/state` fetch settled at 129 bytes of a 385-byte response and
handed back a plausible header followed by 40 bytes of nothing. `NSETTLE` now
also waits for a caller-supplied minimum -- a length the reply cannot be
shorter than -- which removes the guess entirely.

## F8 traps this port paid for

**Unsigned comparisons need the carry, not the sign.** Subtracting and testing
`BM` is a *signed* test: 181 - 1 is 180, bit 7 set, and reads as negative. A
settled 181-byte reply therefore looked short of a 1-byte minimum and the poll
never terminated. `NCMPMIN` tests the carry out instead.

**A 16-bit add has two places a carry can come from.** `LNK` adds the low
half's carry and can itself carry out (`$FF + 1`), and the `AS` that follows
then reports none. Taking only the second loses it -- which made every
subtraction in the decimal formatter appear not to fit, so every number
printed as `0`.

**`PI` and `JMP` clobber the accumulator**, and `LEAVE` ends with a `PI`. No
value survives a call in A.

**The XDC dance has a direction, and getting it backwards is silent.** The
two-data-counter copy idiom is `LM` (read the source) then `XDC / ST / XDC`
(write the destination). `SGMVCODE` did the first `XDC` *before* the `LM`, so it
read the destination and wrote the source -- copying an empty `VMOVE` over the
reply window, which the cart will not let the console write anyway. Nothing
faulted. `BLDURL` then asked for `/move/?table=...`, the server answered 404,
and the next `/state` still said it was your turn: choosing CALL looked like it
did nothing at all, over and over. A copy loop that writes the wrong way round
cannot be seen in a register dump -- only in what came out the far end.

**Draw routines write most of the register file**, so an error code parked in
r1 comes back as part of a screen coordinate. Codes go in RAM.
