# FujiNet 5 Card Stud — Atari 2600

The seventh hand-written client in this family, after the Intellivision,
Arcadia 2001, Astrocade, Odyssey², Channel F and the eleven C platforms — on
a console with **128 bytes of RAM, no framebuffer, one button and four
directions.**

Talks to the cartridge in
[`fujinet-firmware/pico/atari-2600`](../../fujinet-firmware/pico/atari-2600),
and to `https://5card.carr-designs.com/` in the `?bin=1` binary wire format.

```sh
make                       # build/5card.bin, 16384 bytes
make layout                # the screen with no network at all
make run                   # in a window
make drive                 # headless, plays a hand against the live server
make resettest             # the RESET switch, mid-hand
make hosttest              # the cartridge's card art, bit for bit
```

## What is different about this console

Everything follows from two facts, and the second is the one that shapes the
design.

**There are 128 bytes of RAM and the stack mirrors into the top of them.** So
the cartridge holds the state. The server's 418-byte `Game` struct sits in the
cartridge's 512-byte reply window and is read from there *in place* — that
window is 512 bytes and not 256 precisely so that a `Game` and a `Tables` both
land in one slice and no client ever has to page one. The player name and the
table id live in the cartridge's 256-byte path buffers. URLs are streamed a
character at a time straight into the TX page. Nothing in this client is a
buffer, and the two strings it does copy are copied cartridge-to-cartridge.

**There is no framebuffer**, so the cartridge composes the glyphs and
publishes them as six 128-byte planes; the 6507 does nothing but indexed loads
into `GRP0`/`GRP1` on a cycle-exact schedule. 21 rows of 12 columns, a 3×5
glyph in a 4×6 cell.

## The screen

```
 P<pot:5>$<purse:5>          the pot, and YOUR purse
 [c1][c2][c3][c4][c5]  NAME  eight seats, two rows each:
 [p1][p2][p3][p4][p5]  0100    ranks over pips, then the name and the bet
 ...                           -- or the purse, while SELECT is held
 FOLD                        the bottom bar: the move menu, with a
 CALL 100          07        full-width red bar on the selection and the
 RAISE                       move clock; or WAITING ON <name>; or a page
 ALL-IN                      of the end-of-hand banner
```

Eight seats at two rows each is sixteen of the twenty-one, and that is the
whole budget: one row of chrome above and four below. A seat's status —
folded, departed, to act, you — is carried by its **colour** rather than by a
character, and that is what pays for the four-character name and the
four-digit value beside every hand.

### The seam line, which is where the colour comes from

`vcs_render_row()` writes **zero** to scanline 5 of every cell: the sixth line
of a 4×6 cell is blank leading and the renderer does not store it. So the
kernel is 21 rows of (5 cycle-exact ink lines + one **seam line**), and the
seam line is a line on which no glyph can possibly be drawn.

That costs no scanlines — the blank line was already there, and the picture is
still 126 lines — and three facts make its timing a non-problem:

- its sprite writes are *blanking* writes, so the copies draw nothing whatever
  cycle they land on;
- `COLUBK` has until the green playfield border ends at pixel 31, about cycle
  32, because the playfield draws *over* the background — a 33-cycle window
  for a compare and a branch, not the 22 of hblank;
- `COLUP0`/`COLUP1` have no deadline at all on that line, because the sprites
  are blank.

**What is free is the deadlines, not the cycles.** The seam line still has to
finish inside its 76, and this is a `WSYNC`-bounded loop, so one that does not
never glitches — it silently costs a *second* scanline, because the next
`sta WSYNC` is already on the following line by then. The first cut reached
`DROWL` at cycle 77 on the seat path: every seat row was seven scanlines, the
text was 142 lines, the frame 278, and the screen looked entirely correct.
The seat path now lands at 72, with the low `RSEAT0` bound test deleted (it
could never branch — the seam programmes row `CDROW + 1`, which is 1-21), the
row counter advanced with `stx` instead of a five-cycle `inc`, and the chrome
branch moved out of line so the path with no cycles left gets the fall-through.

What it buys: a green frame, a black panel, the red move bar, and a per-seat
ink colour — with the 53-cycle ink body left byte-identical on every line that
draws. **That body is transcribed from the firmware's `fujidisp.inc` and must
not be retimed**; its four constants are the coordinates of a one-pixel-wide
window found by byte-comparing the raster, and every wrong setting still looks
like text. That window is one pixel wide *for text*: for all 48 pixels it is
zero pixels wide, which is what the card bed's left margin below is about.

### The cards

A suit is not in ASCII, and four new font entries do not survive measurement:
a font cell is three pixels wide, and at three pixels a heart, a spade and a
club are very nearly the same picture. So the cartridge gained
**`FN_BLIT_CARD`** — app-specific transforms living in the cart is the
established pattern here, `FN_BLIT_FIELD` and friends are all Battleship's —
and a card ignores the cell grid entirely:

```
 hearts    diamonds   spades    clubs      the ten, the one rank
 .#.#.      ..#..     ..#..     ..#..      3x5 cannot spell
 #####      .###.     .###.     ##.##       #.###
 #####      #####     #####     .###.       #.#.#
 .###.      .###.     ..#..     ..#..       #.#.#
 ..#..      ..#..     .###.     .###.       #.###
```

Five cards on a **six-pixel pitch** — five of art, one of gap — fill pixels
2-30, which is planes 0-3; planes 4 and 5 are columns 8-11 and the cartridge
never touches them, which is what lets a seat carry its name and purse beside
its hand. A card is two text rows tall, the rank over the pip, separated for
free by the seam line. Six stores paint a whole hand.

The bed starts at pixel **2**, and that margin is the one interesting number
here: **pixel 7 — bit 0 of plane 0 — cannot be drawn at all.** Six player
copies 2.67 cycles apart and seven 3-cycle GRP writes leave the four late
writes spanning 33 pixels where only 32 are available, so every block position
loses exactly one pixel, and the position that puts the loss on plane 0 bit 0
is the right one — `vcs_render_row()` already spends that bit as column 1's
inter-character gap, so no text has ever noticed and `dispcheck.py`, which
only renders text, cannot see it. A bed flush to pixel 0 put card slot 1's
*leftmost ink column* there: every slot-1 card came up with its left edge
shaved off, a King rendering as a bare vertical bar. `host_test` now asserts
the invariant against all fourteen ranks and five suits.

Ranks re-centre the cartridge's own 3×5 glyph in the five-wide field rather
than carrying a second alphabet, so a rank and a letter cannot disagree. `"??"`
draws as a hatched back, and `FN_CARD_HIDE0` masks your own hole card until
the showdown.

**Colour per card is not possible and is not attempted.** `COLUP0`/`COLUP1`
are per scanline and cover two fixed interleaved 8-pixel groups, so cards
1/3/5 and 2/4 can differ from each other but never from their own suits. The
suits are carried by shape, at the width at which shape works.

## The banks

Seven 2K banks and the fixed half: 16384 bytes, one of the four sizes MAME's
cart slot accepts. A bank switch replaces every byte of `$1000-$17FF`, so each
bank carries its own copy of every module it calls; only zero page crosses,
and the text planes and the reply window are cartridge state that survives.

| | |
|---|---|
| 0 `cdlobby` | the cold start and the table list |
| 1 `cdgame` | the table: display, render, bet |
| 2 `cdnet` | one request, and back |
| 3 `cdmenu` | the in-game menu, leaving, help |
| 4 `cdname` | the keyboard and the shared username |
| 5 `cdcomp` | compose the whole table, in a frame of exactly 262 scanlines |
| 6 | spare, a stub back to bank 0 |

**The fetch has a bank of its own, and that is why there are seven.**
`net.inc` and `url.inc` are 550 bytes together and the game bank needs every
byte it can get; it came in 552 bytes over with the network in it. A poll
happens every ninety frames, so paying a bank switch for one costs nothing.

**It carries the display kernel too, and that was not the original plan.** A
poll measured a **four-frame gap in `VSYNC` every ninety-four frames** — a
flash, once every second and a half — and the network was barely any of it:
two of those frames were the settle loop's own `#3 × CDWAIT`, a deliberate
delay spun blind in a bank that could not draw, and the rest was the transport
counting down in `FNGO` waiting on `ACKSEQ`. Neither wait emits a `VSYNC`, so
the picture did not go black so much as stop being a picture.

Both waits now go through `NFRAME`, which draws a real frame out of the text
planes the cartridge is still holding — the reply window is not repainted
until the `READ`, so the table on screen stays valid for the whole
transaction. `FNGO` itself could not change (it is in the fixed tail, which
has eight spare bytes, and `cdname` shares it for appkey calls with no picture
to keep), so `net.inc` carries `NPGO`: the same single-commit launch and the
same ~9s timeout in the same `FNTMO` quanta, polled once a frame instead of in
a three-deep counted loop. **The gap is 4.0 frames before and 1.6 after**, and
what is left is not the poll at all — it is `GWARM`'s full recompose, which is
work rather than waiting.

One thing the gate is for: this bank is entered from the lobby as well, and a
bank has room for one `CDBGT`, not two. `NFRAME` draws for `RQSTATE` and
`RQMOVE` — the two requests the game bank issues, and the only ones that
repeat — and keeps the blind delay for the lobby's one-off `RQTABLE` and
`RQLEAVE`, which happen on a screen change where nobody can see a blank.

**`cdcomp` is here for TIMING, not size, which makes it the odd one out.**
Composing twenty-one rows is about 12,900 cycles and a frame's vblank is
2,812 — and the vblank is all the blanked time there is, because `DPADA` and
`DPADB` are the visible green bands. So the recompose after a poll could not
run inside a frame, and running it between frames made that frame about 430
scanlines instead of 262. What that does is push the *next* picture down the
screen by the difference: the table jumped, once a poll. The same thing
happened on a SELECT press, which recomposes sixteen seat rows.

There was nowhere to fix it in bank 1 — `cdgame` had thirty-two bytes left.
Bank 5 has two thousand, and what they buy is a frame skeleton that is 262
scanlines whatever the compose costs: three of `VSYNC`, a `T1024T` window wide
enough to compose in, and a counted `WSYNC` pad for the difference. The screen
is blanked for it, so a poll costs a black frame and **no movement**.

It carries a *copy* of `render.inc` rather than taking it away from bank 1,
because bank 1 still composes the small things itself — the move bar on a
cursor key is a couple of rows and fits the vblank budget with room to spare.
Only the whole table and the eight seats do not. Every bank here carries its
own copy of what it calls; this is that rule, not an exception to it. It has
no display kernel at all: it blanks, composes and leaves.

**The shared transport and the text primitives are in the fixed tail.**
`$1F20-$1FFB` is the one region every bank sees at the same address, so it is
the only place shared code can live; the transport is 137 bytes and would
otherwise exist seven times. `tools/mktail.py` reads the addresses back out of
the tail's own listing into `build/tail.inc`, so there is no hand-maintained
address list to go stale.

A bank switch is a JUMP, never a call — the store that switches is the last
instruction fetched from the old bank — so "where to carry on" is **data**, in
`CDENT`. That is the Odyssey² port's `X_RET` by another name.

## The network layer

Four requests, and `/move/XX` **replaces** `/state` for that poll: its reply
*is* the next state, so there is no separate submit anywhere.

```
N:<endpoint>tables?bin=1
N:<endpoint>state?table=<id>&player=<name>&bin=1
N:<endpoint>move/<CC>?table=<id>&player=<name>&bin=1
N:<endpoint>leave?table=<id>&player=<name>&bin=1
```

Seven rules, each learned the hard way somewhere in this family:

1. **The CLOSE goes at the START of the next request**, never after the READ.
   Every transaction repaints the reply window and every screen renders
   straight out of it between polls.
2. **Capture `RXLEN` immediately** after the READ; the next transaction
   overwrites it with its own, which is always zero.
3. **Settle `NET_STATUS` on two agreeing readings AND a minimum.** Two
   agreeing readings alone is not enough — the Channel F port watched a
   `/state` settle at 129 bytes of a 385-byte response.
4. **Check STATUS byte 3.** The HTTP GET is deferred until the first STATUS
   call, so that byte is the only place an HTTP error is visible; an error
   page has a readable body and a plausible length.
5. **Validate `154 + 33 * playerCount` against the reply length** before
   believing any offset in it.
6. **SEQ comes from the cartridge's own ACKSEQ + 1**, never from a counter in
   RAM. There is no reset line on this connector.
7. **Never read `$1D00-$1EFF`, and only `STA`/`STX`/`STY` may target them.**
   No read-modify-write, no indirect store anywhere. `checkrom.py` is a static
   proof of this, because the 6507 has no interrupts.

No `&be=1`: the Go server's big-endian path is the road much less travelled,
and the Intellivision port found pot and bet corruption on it that always
landed on exact multiples of 256.

## Controls

| | |
|---|---|
| Stick up/down | move the cursor — the move menu, the list, the keyboard |
| Fire | select, submit, sit |
| Stick left | the in-game menu; left again backs out |
| Stick right | poll now |
| **SELECT held** | every seat's value field shows its **purse** |
| RESET | a cold start, back to the lobby. The cartridge survives it — see rule 6 — so the very next transaction continues the conversation rather than colliding with one already answered |

The end-of-hand banner fires on a **checksum of `lastResult`**, not on the
round. Triggering off the round is wrong twice over: at a small table where
everyone folds, the server finishes the hand and deals the next without the
round ever leaving 1, so there is no edge to see — and even at a full table
the showdown can fall between two polls. The banner pages four rows at a time,
breaking on spaces, and stops at the first page with nothing on it.

The table is drawn **before** the banner is considered. The poll carrying the
showdown message is the same one carrying the revealed hands, and by the next
poll the server has dealt again.

## Things that cost real time

- **`OPEN_APPKEY` takes a six-byte packed struct, not four parameters.**
  `AppKeyMixin::appkey_open()` does one `transaction_get()` of
  `sizeof(appkey)` — creator as a u16, then app, key, mode and a **reserved**
  byte that is not optional; leave it off and the firmware waits for a byte
  that never arrives, which reads back as a timeout rather than a protocol
  error. Sent as parameters it is NAK'd, so every boot fell through to the
  keyboard and the shared username was never found. It also fails outright
  when the adapter has no SD card mounted, which is not a client bug, and is
  why the keyboard is the fallback rather than an error screen.
- **RAM is not cleared by a reset, and `CDENT` lives in it.** A restart taken
  at the keyboard came back into the *lobby's warm path*, which redraws a
  table list out of a reply window that is holding a `Game` — so it read table
  names out of the middle of `lastResult`. The cold stub forces `ENCOLD` now.
- **The RESET switch is a SWITCH here**, `SWCHB` bit 0, which the program
  reads. It reboots nothing at all unless the client acts on it, and the first
  reset test pressed it and concluded the client had stopped talking. The test
  uses `soft_reset()`; the client acts on the switch by choice.
- **A bank switch is a JUMP, so nothing ever returns through one** — and the
  switches are taken from inside `APPVBL`, which `DLOOP` reaches with a `JSR`.
  Two bytes of stack leaked per poll, a poll every ninety frames, and after
  thirty-five of them the stack had grown down out of `$C0-$FF` and was
  overwriting `CDMOVE`, `CDENT` and `CDFRAME`. The symptom was a bank entry
  dispatching on an entry code that was really the low byte of a return
  address, about fifty seconds into every session. The trampoline resets `SP`
  now, which is the one place it can be got right once — and paying for those
  three bytes is why `FNCLS` is not in the tail.
- **`checkbanks.py` skipped every address above `$1800`** to ignore the flat
  ROM's vectors, and so reported a bank whose code ran to `$1A28` as fitting.
  It flagged five bytes of a string that straddled the boundary and hid the
  other 550. A check that cannot see the failure it exists for is worse than
  no check, because it is believed. It now skips only segments explicitly
  `ORG`'d above the limit.
- **Included lines in an AS listing carry an `(N)` include-depth prefix.**
  Missing it made the same tool report a 1146-byte image as 331 bytes used.
- **`CDTMP` is not safe across a call.** The per-frame input mask lived there,
  every draw routine borrows it for scratch, and the input tests are
  interleaved with draw calls — so the second test of a frame read a move
  index or a string length as a button press. That walked the move cursor on
  its own and, in the in-game menu, pressed LEAVE.
- **`CDDEC` borrowed `CDTMP` for its field width** and silently ate the seat
  loop's index, so the loop never terminated and the screen was never
  unblanked. It uses no scratch cell at all now.
- **The text planes survive a bank switch.** Returning to the lobby without
  clearing left the game's seat rows, cards and move menu showing under the
  lobby's title and footer.
- **`CDSCOL` is the ink colour for rows 1-16 in every bank**, not just the
  table. The lobby and the keyboard did not fill it, so their rows came out
  black on black and the only legible one was whichever the red bar was under.
- **`IF` conditions must be evaluatable in the assembler's FIRST pass.**
  `CDBANK EQU BANKGAM` placed before the include that defines `BANKGAM`
  assembles every bank with all three request paths in it.
- **The unblank has to land on a line boundary.** Clearing `VBLANK` wherever
  `APPVBL` happened to finish unblanked the raster mid-scanline, and on a
  felt-coloured band that is a visible notch in the corner.
- **`DOWN` out of the keyboard's last grid row** reached OK, DEL or SPC
  depending on the column, and nothing at all from nine of the twelve — which
  reads as a stuck cursor, and put a space on the end of a name that was
  trying to press OK.
- **Throttled is not a performance choice.** The server's start countdown and
  its move clock are wall clock, so an unthrottled run sits at "starting in 2"
  forever and never gets a turn.
- **MAME must run from its own tree** or `-autoboot_script` is silently
  ignored; `SDL_VIDEODRIVER=dummy` wherever there is no `DISPLAY`;
  `fujinet-pc`'s BoIP listener takes one client, so a stray MAME starves the
  next run and the symptom is a hang, not an error.
- **Re-run `emu/apply.sh` after any edit to a shared firmware source.** MAME
  compiles its own copies.

## How it is checked

Nothing here is believed from a screenshot. At 3x5 `S`/`5`, `O`/`0` and
`C`/`[` are one picture, so this family compares **rendered forms** — and the
card art is not on the cell grid at all, so it is compared as **plane bytes**.

| | |
|---|---|
| `make hosttest` | `FN_BLIT_CARD` against a 32-pixel *picture* of the bed, packed by the test's own loop. It caught `card_suits[]` spelling "hdcs" against a pip table ordered hearts, diamonds, spades, clubs — every club drew as a spade — on the four bytes where those two pips differ and nowhere else. |
| build gates | `checkdefs.py` (the client's equates against the firmware header), `checkrom.py` (image size, the `"FUJI"` claim, the reset vector in the fixed half, no RMW or indirect store on the control pages), `checkbanks.py` per bank and `mktail.py` on the tail. All fail the build. |
| `make layout` | the kernel, the colours and the 21-row geometry with no network at all — including all eight seats, which a live table does not always fill. |
| `make drive` | types a name, sits at a real table on `5card.carr-designs.com`, and plays. It dumps the reply's `hand[11]` and the card bed's plane bytes so the art can be checked against live data: a `6h 2h kc 9d` came back as the hatched back, then a heart, a club and a diamond pip, all twelve bytes exact. |
| `make resettest` | pulses the RESET switch mid-hand and asserts the cartridge's sequence carried across it. |

The card bed decoded out of a live game, for `hand[11] = "6h2hkc9d"` with the
hole card masked:

```
 plane 0  1  2  3       rank row            pip row
 F9 C5 1C 00            #####  .###.        #.#.# .#.#.
 A8 45 14 00            #.#.#  ...#.        .#.#. #####
 51 C6 1C 00            .#.#.  .###.        #.#.# #####
 A9 05 04 00            #.#.#  .#...        .#.#. .###.
 51 C5 1C 00            .#.#.  .###.        ##### ..#..
 00 00 00 00            (the seam line, always blank)
```

## Not here

- **Colour per suit.** See above; it is not reachable on this hardware.
- **Real hardware.** The RP2040 firmware builds and `checksram.py` passes, but
  bus timing is the one thing emulation cannot settle — the same position the
  Channel F port is in.
- **PAL.** The kernel's line counts are NTSC and its constants are a
  one-pixel-wide window.
- **Dealing animations.** No sibling under 40 columns has them; a per-round
  click stands in.
