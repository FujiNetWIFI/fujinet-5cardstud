# 5 Card Stud for the Bally Astrocade

A standalone Z80 assembly client, in the `o2/` mold: the shared C core
cannot fit this machine, so the game is written to it. It talks to the
5 Card Stud server through the FujiNet Astrocade cartridge — the RP2040
mailbox cart from `fujinet-firmware/pico/astrocade` — and renders with
the Lynx port's screen plan, because the Lynx's 160x102 display is this
hardware's exact resolution.

Built and tested against the live server (`https://5card.carr-designs.com/`)
in MAME, through a real fujinet-pc-rs232.

## Building and running

    ./build.sh                # build/5card.bin, exactly 8192 bytes
    ./run.sh                  # MAME with the fujinet cart device
    make smoke                # headless end-to-end test (see below)

Environment:

  * `ENDPOINT=` — game server, default `https://5card.carr-designs.com/`.
    Regenerated into `build/endpoint.inc` on every build.
  * `DEMO=1` — assemble the static mock table (`demo.inc`) instead of the
    game: every drawing path with zero network, for art and palette work.
  * `FUJI_FIRMWARE=` — the fujinet-firmware checkout (default
    `~/Workspace/fujinet-firmware`). Supplies the vendored zmac and
    `tools/checkrom.py`; the toolchain and the window-layout contract
    stay in one place instead of drifting in a copy here.

`run.sh` expects the MAME tree with the fujinet cart device grafted in
(`pico/astrocade/emu/apply.sh`) at `MAME_DIR` (default `~/Workspace/mame`)
and a fujinet-pc BoIP listener at `FUJINET_TCP` (default 127.0.0.1:9995).
At the on-screen menu, keypad **1** starts the game.

`make assets` regenerates `assets/font.inc` and `assets/cardart.inc` from
the Lynx port's art (`tools/mkfont.py`, `tools/mkcards.py`); the outputs
are committed, so this is only needed after art changes.

## The cartridge budget

The cart serves an 8K window; the mailbox owns 1B00H up, so code and data
end at 1AFFH — 6,912 bytes, enforced by `checkrom.py` and itemised by
`tools/checksize.py` on every build (the `MB_*` labels in `5card.asm` are
its module fences). `build.sh` stamps the `FUJI` claim signature at
1CFCH, so when this image is booted over the network the cart keeps the
mailbox alive for it.

RAM is screen RAM, full stop. 90 visible lines use 4000H–4E0FH and
everything above is the game's:

    4000-4E0F  bitmap, 90 lines x 40 bytes
    4E10-4E6F  poll edges, menu and cue state, session state (see 5card.asm)
    4E70-4EFF  table name, hand buffer, name-entry buffer
    4F00-4F2F  LINBUF (fujicfg convention)
    4F40-4FBF  stack (SP = 4FC0H)
    4FC0-4FFF  left clear: BIOS cells and the boot stub's transient home

Interrupts stay off for the program's whole life (fujilib's contract:
with I = 0, refresh strays land in OS ROM and never hit the hotspots).

## Graphics

Everything rides one alignment trick: at 2bpp, four pixels are one byte,
and the whole layout lives on a 4-pixel grid — so no blit in the game
ever shifts. Text is the Lynx 4x6 font, 1bpp, expanded to any fg/bg pair
by the magic expander (one MAGIC-register write per row keeps its nibble
flip-flop on the high nibble). The screen is 40x15 character cells; the
palette is four colors from the 256-color hardware palette: felt green,
black, red, white (`PALET` in 5card.asm — tune it in the DEMO build).

Cards are 12x17 pixels on an 8-pixel pitch, drawn left to right, so a
covered card shows its left 8 pixels: the black seam column at px0, the
rank (a plain font glyph at px4-7, red or black on white), and the suit
(a raw 2bpp stamp, ink at px1-7). The ten is the one rank that needs two
glyphs, so it is a raw stamp too, composited by `mkcards.py` from the
font's own 1 and 0. Face-down cards and the hole-card sliver are raw
stamps with the seam baked in. Seat geometry is the Lynx port's
(`vars.c`), compressed from 17 to 15 rows, with `PCIDX` choosing seats by
player count and you always bottom-center.

Rendering policy: every poll redraws everything, opaquely, straight out
of the reply window — rewriting identical pixels is invisible, so there
is no flicker and no dirty-rectangle bookkeeping. The one thing opaque
overdraw cannot fix is content that shrank (a new hand dealt, a seat
emptied), so `CHKNEW` watches round/playerCount and answers with one full
clear + statics redraw.

## The network layer

`net.inc` is a Z80 transcription of `o2/net.inc`'s proven sequence over
`fujilib.inc` (the mailbox client, copied from the firmware tree). The
rules it carries forward, each learned the hard way on the O2 or the
Intellivision:

  * **CLOSE opens the next request, never ends this one.** Every
    transaction repaints the reply window, a CLOSE reply is empty, and
    every screen renders out of the window between polls.
  * The reply length is captured immediately after the READ.
  * STATUS is polled until two consecutive readings agree (bounded),
    because the ESP32 reports bytes as soon as SOME of the response has
    arrived.
  * STATUS reply byte 3 is nDevStatus_t and gets checked: an HTTP error
    page still has a readable body.
  * The reply length is validated against what playerCount implies
    before a byte of it is believed.
  * URLs are streamed into the TX byte stream a character at a time,
    never assembled in RAM; fixed-width wire fields are pushed up to
    their NUL, never through it.

The reply is parsed in place through `state.inc`'s flat 0–1023 cursor
over the 256-byte slice window (`RXGETB`/`RXGETW`/`RXSTRN` switch slices
only at 256-byte boundaries). Nothing is buffered but a name here and a
hand there.

Sequence numbers come from the cart's persisted ACKSEQ, never a local
counter, so console RESET mid-hand restarts the program while the
transaction stream continues unbroken — `emu/resettest.lua` proves it.

## Choosing a move and sound

* **Choosing a move.** The menu has no digits: the highlighted entry
  (white on black) is moved with the stick, one step per push, or by
  twisting the knob, which splits the menu into equal arcs of its travel,
  and the trigger bets it. The cursor starts on the second move
  (call/check) each turn, as in the C clients. The knob only takes over
  once it has turned 6 steps from where it rested, so it neither jitters
  between neighbours nor snaps back over a choice made with the stick.
* **Sound.** Every cue in `src/platform-specific/sound.h` is here, with
  the Intellivision port's pitches (`sound.inc` is shared verbatim with
  the Texas Hold'em client): join, your turn, the tick of your move clock
  (each poll that changes it), cursor and cursor-at-an-end, select, a
  noise click per card dealt, the chip sweep at a new round, player
  joined/left, and the showdown fanfare. Event cues are queued while
  `CHKNEW` examines the poll and played after the render, so the news is
  on screen when it sounds.

## Controls

    stick / keypad arrows   move through lists, turn the name wheel
    stick left/right, knob  move the highlight along the move menu
    trigger                 select / join / accept / bet the highlighted move
    keypad 0                poll now
    CE                      leave the table (name screen from the list)
    .                       how to play

## Testing

`make smoke` runs MAME headless with `emu/smoke.lua`: launch from the OS
menu, accept the default name, join a table by keypad digit, then every 3
seconds twist the knob and pull the trigger so some presses land inside
real move windows (the AI ROOM bots keep the hand moving), snapshot to
`build/astrocde/0000.png`.
`FUJINET_DEBUG=1` (default) logs every mailbox transaction; a `/move`
shows as an OPEN with `txlen=69` against `/state`'s 67.

`emu/resettest.lua` is the RESET-continuity check: join, soft-reset the
console mid-session, rejoin, and confirm the sequence numbers continue
instead of restarting — the autoboot script re-runs after a soft reset,
which is why its timing is relative and only the first run pulls the
reset.

The network-boot path is the cartridge bring-up's: stage `build/5card.bin`
on a host the fujinet-pc can reach, boot it with `fujicfg` (browse and
select) or a `BOOT_PATH=/5card.bin` `fujiboot`, and the cart DBC-pushes
the image, swaps it in, and — because of the claim signature — keeps the
mailbox alive for it.

## Status

Working end to end against the live server: table list, join, live
rendering of 7-player bot tables across rounds and showdowns, moves,
leave, the last-result banner, the full cue set, RESET continuity, and
booting over the network. Not yet done: appkey persistence for the
username (no Astrocade lobby exists to share it with yet), and nothing
has run on real hardware, because the cartridge itself has not been
built — see the bring-up README's hardware notes.

## Bank switching

Firmware protocol v2 supports banked carts: `fujilib.inc` now carries the
`FNBKSEL`/`FNBKMAX` equates (one read maps a 4K image page into
2000H-2FFFH with the mailbox fully live; the high half never moves). This
client still fits the single 8K window and does not use them -- see
`firmware/include/fuji_mailbox.h` in fujinet-firmware for the scheme.
