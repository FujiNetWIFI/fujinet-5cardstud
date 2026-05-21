#!/usr/bin/env python3
"""
Convert 5cardstud's pmode3.fnt (2bpp, 8 bytes/glyph, 4x8 pixels) to a 4bpp
charset (32 bytes/glyph, 8x8 pixels) suitable for the CoCo 3 320x200x16
graphics mode used by the new port.

Source format:
  - 128 glyphs, 8 bytes each = 1024 bytes total
  - Each byte = 4 pixels at 2bpp (high bits = leftmost pixel)
  - Pixel value 0-3 is a palette index into the 4-color CoCo 1/2 palette

Output format:
  - 128 glyphs, 32 bytes each = 4096 bytes total
  - Each glyph: 8 rows x 4 bytes/row, 4bpp (high nibble = leftmost pixel)
  - Each source pixel becomes 2 destination pixels of the same colour
    (horizontal pixel-doubling). The 2-bit source value is stored in the
    low 2 bits of each 4-bit destination nibble; the high 2 bits are 0,
    so colours 0-3 map 1:1 into palette entries 0-3 in 16-colour mode.
"""

import sys
import os

src_path = os.path.join(os.path.dirname(__file__), 'pmode3.fnt')
dst_path = os.path.join(os.path.dirname(__file__), 'pmode3_4bpp.fnt')

with open(src_path, 'rb') as f:
    data = f.read()

if len(data) != 1024:
    sys.exit(f"expected 1024 bytes in {src_path}, got {len(data)}")

out = bytearray()
for glyph in range(128):
    g = data[glyph * 8 : glyph * 8 + 8]
    for row_byte in g:
        # 4 source pixels, leftmost in the high bits
        p0 = (row_byte >> 6) & 3
        p1 = (row_byte >> 4) & 3
        p2 = (row_byte >> 2) & 3
        p3 = row_byte & 3
        # Each source pixel -> 2 destination pixels (horizontal doubling),
        # both nibbles of the output byte hold the same value.
        out.append((p0 << 4) | p0)
        out.append((p1 << 4) | p1)
        out.append((p2 << 4) | p2)
        out.append((p3 << 4) | p3)

with open(dst_path, 'wb') as f:
    f.write(bytes(out))

print(f"Wrote {len(out)} bytes to {dst_path}")
