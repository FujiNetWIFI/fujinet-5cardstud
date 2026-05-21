#!/usr/bin/env python3
"""
Build the CoCo 3 16-byte/glyph charset for the 320x200x16 build.

Output layout (matches fujitzee's charset_coco3 format):
  128 glyphs x 8 rows x 2 bytes/row, CGA 2bpp, 8 pixels wide x 8 tall.

Strategy:
  - For every code point, start from the matching glyph in
    support/coco/pmode3.fnt (5cardstud's 4-pixel-wide font) and pixel-
    double horizontally so each 4-pixel row becomes the equivalent
    8-pixel row in 2bpp.
  - For uppercase ASCII positions 0x41..0x5A, overwrite with the
    higher-resolution uppercase glyphs that fujitzee stores at the
    matching LOWERCASE positions (0x61..0x7A). These are native 8px-wide
    so they retain their detail.
  - 5cardstud-specific glyphs (card art at 0x01-0x10, box-drawing at
    0x3B-0x40, back-of-card pattern at 0x1E/0x1F, wavy border at
    0x74-0x77, chip 0x22, clock 0x28, etc.) come straight from the
    pixel-doubled pmode3 - their layout is unchanged.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
PMODE3      = os.path.join(ROOT, "support", "coco", "pmode3.fnt")
FUJITZEE_H  = os.path.join(ROOT, "src", "coco", "charset_coco3.h")
OUTPUT      = os.path.join(ROOT, "support", "coco", "pmode3_coco3.fnt")

N_GLYPHS = 128


def pixel_double_byte(src_byte):
    """One 4-pixel 2bpp byte -> two 4-pixel 2bpp bytes (8 pixels total),
    with each source pixel duplicated horizontally."""
    p0 = (src_byte >> 6) & 0x3
    p1 = (src_byte >> 4) & 0x3
    p2 = (src_byte >> 2) & 0x3
    p3 = src_byte & 0x3
    hi = (p0 << 6) | (p0 << 4) | (p1 << 2) | p1
    lo = (p2 << 6) | (p2 << 4) | (p3 << 2) | p3
    return hi, lo


def pixel_double_glyph(eight_bytes):
    """8-byte 4x8 glyph -> 16-byte 8x8 glyph."""
    out = []
    for b in eight_bytes:
        hi, lo = pixel_double_byte(b)
        out.extend((hi, lo))
    return out


def load_pmode3():
    with open(PMODE3, "rb") as f:
        data = f.read()
    if len(data) != 1024:
        sys.exit(f"expected 1024 bytes in {PMODE3}, got {len(data)}")
    return [list(data[i * 8 : i * 8 + 8]) for i in range(N_GLYPHS)]


def load_fujitzee():
    """Parse charset_coco3.h. Returns 128 lists of 16 ints."""
    with open(FUJITZEE_H) as f:
        text = f.read()
    entries = re.findall(r'/\*\s+0x[0-9A-Fa-f]+\s+\*/\s+\{([^}]+)\}', text)
    glyphs = []
    for body in entries[:N_GLYPHS]:
        bs = [int(b, 16) for b in re.findall(r'0x[0-9A-Fa-f]+', body)]
        if len(bs) != 16:
            sys.exit(f"fujitzee glyph wrong size: {len(bs)}")
        glyphs.append(bs)
    if len(glyphs) != N_GLYPHS:
        sys.exit(f"fujitzee had {len(glyphs)} glyphs, expected {N_GLYPHS}")
    return glyphs


def main():
    pmode3   = load_pmode3()
    fujitzee = load_fujitzee()

    # Fujitzee draws every letter as pixel value 3 (which renders via
    # fujitzee's text_palettes lookup as "white"). 5cardstud renders
    # pixel-value-N directly through palette[N], where pixel 3 = red.
    # Remap fujitzee's pixels into 5cardstud's convention so an
    # uppercase letter ends up as black-on-felt like all other text:
    #   fuji 0 (bg) -> 0 (felt)
    #   fuji 1 (gold secondary) -> 0 (drop; we don't have a useful
    #                                  secondary colour for text)
    #   fuji 2 (black) -> 2 (black, unchanged)
    #   fuji 3 (white letter shape) -> 2 (black, becomes the letter)
    FUJITZEE_LETTER_REMAP = (0, 0, 2, 2)

    def remap(bs, table):
        out = []
        for b in bs:
            r = 0
            for shift in (6, 4, 2, 0):
                pix = (b >> shift) & 3
                r |= table[pix] << shift
            out.append(r)
        return out

    # Fujitzee's high-res digits 0-9 actually live at positions 0x10-0x19
    # (the MSDOS source labels them "// 30".."// 39" by glyph, not by
    # array index, and cvt_msdos_font.py preserves the array order).
    # Position 0x30-0x39 in charset_coco3.h contains unrelated decorative
    # blocks; ignore those.
    output = []
    for c in range(N_GLYPHS):
        if 0x41 <= c <= 0x5A:
            output.extend(remap(fujitzee[c - 0x41 + 0x61],
                                FUJITZEE_LETTER_REMAP))
        elif 0x30 <= c <= 0x39:
            output.extend(remap(fujitzee[c - 0x30 + 0x10],
                                FUJITZEE_LETTER_REMAP))
        else:
            # Card art, box-drawing, suits, punctuation, partial-card
            # variants, wavy border, etc. - keep 5cardstud's pmode3
            # glyph pixel-doubled to 8 px.
            output.extend(pixel_double_glyph(pmode3[c]))

    assert len(output) == N_GLYPHS * 16
    with open(OUTPUT, "wb") as f:
        f.write(bytes(output))
    print(f"wrote {OUTPUT} ({len(output)} bytes, {N_GLYPHS} glyphs)")


if __name__ == "__main__":
    main()
