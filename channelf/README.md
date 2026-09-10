# 5 Card Stud for the Fairchild Channel F

A standalone F8 client, in the Arcadia and Odyssey&sup2; mould: the shared C
core cannot fit these machines, so the game is written to them.

    make            # -> build/5card.bin, 16384 bytes

**4,767 bytes** in a 16K window, next to the Arcadia port's 4,509 for the same
job. Verified against the live server at `5card.carr-designs.com`:
`emu/5carddrive.lua` in the firmware tree types a name on the on-screen
keyboard, commits it, lists the real tables, sits down, and renders a live
seven-player hand.

## What is different here

RAM is not the constraint. The Arcadia client runs in 84 bytes and the
Odyssey&sup2; client in 39, because neither cartridge edge can accept a write
cycle; this one can, so the cart hands the console 30K. The screens still
render straight out of the mailbox's reply window rather than buffering the
wire format -- not to save RAM, but because it is simpler and the
reply-stability invariant makes it free.

## Screen

23 columns x 9 rows. One row per player, 22 cells after the cursor gutter:

    NAME__ ppp CCCCCCCCCC

six of name, three of purse, five cards as two ASCII bytes each. The server
sends cards already as text (`as`, `th`), so they are drawn as they arrive;
`??` is a hole card. The player to act is drawn in the accent colour.

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

**Draw routines write most of the register file**, so an error code parked in
r1 comes back as part of a screen coordinate. Codes go in RAM.
