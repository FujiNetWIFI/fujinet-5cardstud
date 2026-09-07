#!/usr/bin/env python3
"""Convert the MS-DOS CGA 2bpp tile set to TMS9918 1bpp assets.

Shared by every TMS9918 client in this tree - the Coleco Adam and the MSX use
the same video chip and the same z88dk --generic-console driver, so they get
byte-identical art.

Reads  src/msdos/charset.h  (8x8 tiles, 16 bytes each: 2 bytes/row, 4px/byte,
CGA colors 0=black 1=green-background 2=red 3=white) and writes, into each of
OUT_DIRS below:

  font.bin  768 bytes: the Namco arcade font, glyphs 0x20-0x7F, 1bpp, bit set
            where the CGA pixel is ink (color 0).
  udg.h     UDG pattern table for codepoints 0x80+, 8 bytes/glyph, consumed via
            IOCTL_GENCON_SET_UDGS. Foreground/background colors are applied per
            cell at draw time, so each tile is reduced to ink (bit=1) vs paper
            (bit=0) with a per-array ink predicate (see TILE_MAP below).

Outputs are checked in; rerun from the repo root after art changes:

  python3 support/tms9918/convert-tiles.py
"""

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
CHARSET = REPO / "src/msdos/charset.h"
OUT_DIRS = [REPO / "src/adam", REPO / "src/msx", REPO / "src/coleco"]

EXPECTED_COUNTS = {
    "ascii": 96,
    "card_edges": 9,
    "pot_border": 6,
    "chip": 2,
    "border": 4,
}


def parse_arrays(text):
    """Return {name: [tile, ...]} where tile is a list of 16 ints."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)

    arrays = {}
    for m in re.finditer(r"unsigned\s+char\s+(\w+)\s*\[[^\]]*\]\s*\[16\]\s*=", text):
        name = m.group(1)
        open_brace = text.index("{", m.end())
        depth = 0
        for i in range(open_brace, len(text)):
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
                if depth == 0:
                    body = text[open_brace : i + 1]
                    break
        else:
            sys.exit(f"unbalanced braces in array {name}")
        vals = [int(v, 16) for v in re.findall(r"0x[0-9a-fA-F]+", body)]
        if len(vals) % 16:
            sys.exit(f"{name}: {len(vals)} bytes is not a multiple of 16")
        arrays[name] = [vals[i : i + 16] for i in range(0, len(vals), 16)]
    return arrays


def pixels(tile):
    """Yield 8 rows of 8 CGA pixel values (0-3), left to right."""
    for r in range(8):
        row = []
        for b in (tile[2 * r], tile[2 * r + 1]):
            for i in range(4):
                row.append((b >> (6 - 2 * i)) & 3)
        yield row


def to1bpp(tile, ink_pred):
    out = []
    for row in pixels(tile):
        byte = 0
        for i, px in enumerate(row):
            if ink_pred(px):
                byte |= 0x80 >> i
        out.append(byte)
    return out


INK_RED = lambda px: px == 2
INK_NOT_WHITE = lambda px: px != 3
INK_BLACK = lambda px: px == 0
INK_GREEN = lambda px: px == 1

# codepoint: (#define name, source array, index, ink predicate, note)
# Codepoints are chosen so each platform's graphics.c can keep the original
# codepoint arithmetic (ranks at 0xAA+, suits 0xB7-0xBA, chip 0xBC, ...).
TILE_MAP = {
    0x80: ("UDG_CARD_TL", "card_edges", 0, INK_RED, "card top-left corner"),
    0x81: ("UDG_CARD_BL", "card_edges", 1, INK_RED, "card bottom-left corner"),
    0x82: ("UDG_CARD_TOP", "card_edges", 2, INK_RED, "card top edge"),
    0x83: ("UDG_CARD_BOT", "card_edges", 3, INK_RED, "card bottom edge"),
    0x84: ("UDG_CARD_TOP_TRIM", "card_edges", 4, INK_RED, "top edge, right px trimmed"),
    0x85: ("UDG_CARD_BOT_TRIM", "card_edges", 5, INK_RED, "bottom edge, right px trimmed"),
    0x86: ("UDG_CARD_VERT", "card_edges", 6, INK_RED, "vertical rule (= card_bits[0])"),
    0x96: ("UDG_CARD_BR_STUB", "card_edges", 7, INK_RED, "bottom-right joint"),
    0x97: ("UDG_CARD_TR_STUB", "card_edges", 8, INK_RED, "top-right joint"),
    0x9B: ("UDG_BACK_RCOL_TOP", "card_bits", 1, INK_RED, "partial-right back, top"),
    0x9C: ("UDG_BACK_RCOL_MID", "card_bits", 2, INK_RED, "partial-right back, middle"),
    0x9D: ("UDG_BACK_RCOL_BOT", "card_bits", 3, INK_RED, "partial-right back, bottom"),
    0x9E: ("UDG_BACK_L_TOP", "card_bits", 6, INK_RED, "back hatch left col, top"),
    0x9F: ("UDG_BACK_R_TOP", "card_bits", 7, INK_RED, "back hatch right col, top"),
    0xA0: ("UDG_BACK_L_MID", "card_bits", 8, INK_RED, "back hatch left col, middle"),
    0xA1: ("UDG_BACK_R_MID", "card_bits", 9, INK_RED, "back hatch right col, middle"),
    0xA2: ("UDG_BACK_L_BOT", "card_bits", 10, INK_RED, "back hatch left col, bottom"),
    0xA3: ("UDG_BACK_R_BOT", "card_bits", 11, INK_RED, "back hatch right col, bottom"),
    0xA4: ("UDG_BOX_TL", "pot_border", 0, INK_RED, "box top-left"),
    0xA5: ("UDG_BOX_TR", "pot_border", 2, INK_RED, "box top-right"),
    0xA6: ("UDG_BOX_H", "pot_border", 1, INK_RED, "box horizontal"),
    0xA7: ("UDG_BOX_BL", "pot_border", 3, INK_RED, "box bottom-left"),
    0xA8: ("UDG_BOX_BR", "pot_border", 4, INK_RED, "box bottom-right"),
    0xA9: ("UDG_BOX_V", "pot_border", 5, INK_RED, "box vertical"),
    0xB7: ("UDG_SUIT_SPADE", "black_card_front", 14, INK_NOT_WHITE, "spade"),
    0xB8: ("UDG_SUIT_CLUB", "black_card_front", 15, INK_NOT_WHITE, "club"),
    0xB9: ("UDG_SUIT_DIAMOND", "red_card_front", 14, INK_NOT_WHITE, "diamond"),
    0xBA: ("UDG_SUIT_HEART", "red_card_front", 15, INK_NOT_WHITE, "heart"),
    0xBC: ("UDG_CHIP", "chip", 0, INK_RED, "pot chip (white X becomes cutout)"),
    0xBF: ("UDG_HIDDEN_L", "card_bits", 12, INK_NOT_WHITE, "hole-card marker, left"),
    0xC0: ("UDG_HIDDEN_R", "card_bits", 13, INK_NOT_WHITE, "hole-card marker, right"),
    0xC1: ("UDG_SCREEN_TL", "border", 0, INK_GREEN, "screen corner top-left"),
    0xC2: ("UDG_SCREEN_TR", "border", 1, INK_GREEN, "screen corner top-right"),
    0xC3: ("UDG_SCREEN_BL", "border", 2, INK_GREEN, "screen corner bottom-left"),
    0xC4: ("UDG_SCREEN_BR", "border", 3, INK_GREEN, "screen corner bottom-right"),
}

# Ranks come from the black set: red_card_front[5] is a known MS-DOS art bug
# (a duplicate of the "5" glyph where the "6" should be).
RANKS = "23456789TJQKA"
for i, r in enumerate(RANKS):
    TILE_MAP[0xAA + i] = (f"UDG_RANK_{r}", "black_card_front", 1 + i, INK_NOT_WHITE, f"rank {r}")

LAST_CODE = max(TILE_MAP)


def art_comment(code, name, glyph, note):
    lines = [f"    /* 0x{code:02X} {name}", f"       {note}"]
    for byte in glyph:
        lines.append("       " + "".join("X" if byte & (0x80 >> i) else "." for i in range(8)))
    lines.append("    */")
    return "\n".join(lines)


def main():
    arrays = parse_arrays(CHARSET.read_text())

    for name, count in EXPECTED_COUNTS.items():
        got = len(arrays[name])
        if got != count:
            sys.exit(f"{name}: expected {count} tiles, parsed {got}")
    for name in ("red_card_front", "black_card_front"):
        if len(arrays[name]) != 16:
            sys.exit(f"{name}: expected 16 tiles, parsed {len(arrays[name])}")
    if len(arrays["card_bits"]) < 14:
        sys.exit(f"card_bits: expected at least 14 tiles, parsed {len(arrays['card_bits'])}")

    # Rank glyph shapes should match between the red and black sets so one UDG
    # can serve both (ink color is chosen per cell at draw time). Index 5 is a
    # known mismatch: red_card_front[5] duplicates the "5" where "6" belongs.
    for i in range(1, 14):
        red = to1bpp(arrays["red_card_front"][i], INK_NOT_WHITE)
        black = to1bpp(arrays["black_card_front"][i], INK_NOT_WHITE)
        if red != black and i != 5:
            sys.exit(f"rank glyph {i} differs between red_card_front and black_card_front")

    # card_bits[0] must reduce to the same glyph as card_edges[6] (shared UDG).
    if to1bpp(arrays["card_bits"][0], INK_RED) != to1bpp(arrays["card_edges"][6], INK_RED):
        sys.exit("card_bits[0] no longer matches card_edges[6] as 1bpp")

    font = bytearray()
    for tile in arrays["ascii"]:
        font.extend(to1bpp(tile, INK_BLACK))
    if len(font) != 768:
        sys.exit(f"font.bin: expected 768 bytes, produced {len(font)}")
    for out_dir in OUT_DIRS:
        (out_dir / "font.bin").write_bytes(font)

    defines = []
    chunks = []
    for code in range(0x80, LAST_CODE + 1):
        if code in TILE_MAP:
            name, array, idx, ink, note = TILE_MAP[code]
            glyph = to1bpp(arrays[array][idx], ink)
            defines.append(f"#define {name} 0x{code:02X}")
            chunks.append(
                art_comment(code, f"{name} ({array}[{idx}])", glyph, note)
                + "\n    "
                + ", ".join(f"0x{b:02X}" for b in glyph)
                + ","
            )
        else:
            chunks.append(f"    /* 0x{code:02X} unused */\n    " + ", ".join(["0x00"] * 8) + ",")

    udg_h = (
        "#ifndef UDG_H\n"
        "#define UDG_H\n"
        "\n"
        "/**\n"
        " * @brief 5 Card Stud UDGs for TMS9918 GRAPHICS II\n"
        " * @verbose GENERATED by support/tms9918/convert-tiles.py from the MS-DOS\n"
        " *          CGA tiles in src/msdos/charset.h - do not edit by hand.\n"
        " *          1 bit per pixel; ink and paper colors are applied per cell\n"
        " *          at draw time by the platform's graphics.c.\n"
        " */\n"
        "\n" + "\n".join(defines) + "\n"
        "\n"
        "static const char udg[] =\n"
        "{\n" + "\n\n".join(chunks) + "\n"
        "};\n"
        "\n"
        "#endif /* UDG_H */\n"
    )
    for out_dir in OUT_DIRS:
        (out_dir / "udg.h").write_text(udg_h)

    for out_dir in OUT_DIRS:
        rel = out_dir.relative_to(REPO)
        print(f"wrote {rel}/font.bin ({len(font)} bytes)")
        print(f"wrote {rel}/udg.h ({LAST_CODE - 0x80 + 1} glyphs, 0x80-0x{LAST_CODE:02X})")


if __name__ == "__main__":
    main()
