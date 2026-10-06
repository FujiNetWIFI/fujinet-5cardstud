#!/usr/bin/env python3
"""mktiles.py -- build the Sega Master System Mode 4 tile set for 5 Card Stud.

Converted straight from the MS-DOS CGA art in src/msdos/charset.h, whose four
colours (black, felt, red, white) map onto Mode 4 palette indices, so the SMS
table looks like the MS-DOS one. Each tile is stored as 2bpp plus one truth
table per Mode 4 bitplane, which vdp_tiles() (src/sms/vdp.asm) expands at
start-up, so a tile may use at most four distinct palette indices.

Text tiles sit at their ASCII codes and card art at the UDG codes the TMS9918
ports use, so a name table entry's low byte is the glyph wherever there is
only one colouring. Variants live in the upper 256 tiles (name table bit 8).

Writes src/sms/tiles.h, src/sms/tileset.c and support/sms/tilemap.lua (tile ->
character, for the MAME smoke test). Run by hand after changing the art; the
outputs are committed.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CHARSET = os.path.join(HERE, '..', 'msdos', 'charset.h')
TILES_H = os.path.join(HERE, 'tiles.h')
TILESET_C = os.path.join(HERE, 'tileset.c')
TILEMAP = os.path.join(HERE, '..', '..', 'support', 'sms', 'tilemap.lua')

# Palette indices. Palette 0 is the table; palette 1 (the sprite palette, which
# the background may also select per cell) is the status bar and the viewer's
# hidden hole card: PAPER turns black and FACE turns grey there, nothing else
# changes. See initGraphics().
PAPER, WHITE, RED, BLACK, FACE, FELT, GOLD = 0, 1, 2, 3, 4, 5, 6
PALETTE0 = [0x18, 0x3F, 0x02, 0x00, 0x3F, 0x18, 0x0F]
PALETTE1 = [0x00, 0x3F, 0x02, 0x00, 0x2A, 0x18, 0x0F]

CGA_ART = {0: BLACK, 1: FELT, 2: RED, 3: FACE}
CGA_TEXT = {0: WHITE, 1: PAPER}
CGA_HILITE = {0: BLACK, 1: GOLD}

RANKS = '23456789TJQKA'

UDG = [
    # glyph, name, source array, index
    (0x80, 'CARD_TL', 'card_edges', 0),
    (0x81, 'CARD_BL', 'card_edges', 1),
    (0x82, 'CARD_TOP', 'card_edges', 2),
    (0x83, 'CARD_BOT', 'card_edges', 3),
    (0x84, 'CARD_TOP_TRIM', 'card_edges', 4),
    (0x85, 'CARD_BOT_TRIM', 'card_edges', 5),
    (0x86, 'CARD_VERT', 'card_edges', 6),
    (0x96, 'CARD_BR_STUB', 'card_edges', 7),
    (0x97, 'CARD_TR_STUB', 'card_edges', 8),
    (0x9B, 'BACK_RCOL_TOP', 'card_bits', 1),
    (0x9C, 'BACK_RCOL_MID', 'card_bits', 2),
    (0x9D, 'BACK_RCOL_BOT', 'card_bits', 3),
    (0x9E, 'BACK_L_TOP', 'card_bits', 6),
    (0x9F, 'BACK_R_TOP', 'card_bits', 7),
    (0xA0, 'BACK_L_MID', 'card_bits', 8),
    (0xA1, 'BACK_R_MID', 'card_bits', 9),
    (0xA2, 'BACK_L_BOT', 'card_bits', 10),
    (0xA3, 'BACK_R_BOT', 'card_bits', 11),
    (0xA4, 'BOX_TL', 'pot_border', 0),
    (0xA5, 'BOX_TR', 'pot_border', 2),
    (0xA6, 'BOX_H', 'pot_border', 1),
    (0xA7, 'BOX_BL', 'pot_border', 3),
    (0xA8, 'BOX_BR', 'pot_border', 4),
    (0xA9, 'BOX_V', 'pot_border', 5),
] + [(0xAA + i, 'RANK_' + r, 'black_card_front', 1 + i) for i, r in enumerate(RANKS)] + [
    (0xB7, 'SUIT_SPADE', 'black_card_front', 14),
    (0xB8, 'SUIT_CLUB', 'black_card_front', 15),
    (0xB9, 'SUIT_DIAMOND', 'red_card_front', 14),
    (0xBA, 'SUIT_HEART', 'red_card_front', 15),
    (0xBC, 'CHIP', 'chip', 0),
    (0xC1, 'SCREEN_TL', 'border', 0),
    (0xC2, 'SCREEN_TR', 'border', 1),
    (0xC3, 'SCREEN_BL', 'border', 2),
    (0xC4, 'SCREEN_BR', 'border', 3),
]

# drawLine() off the status row: a bar across the top of the row under the
# active player's name -- blank cells, or the tops of that player's cards.
TOPBAR = ['CARD_TL', 'CARD_TOP', 'CARD_TR_STUB', 'CARD_TOP_TRIM']


def parse_arrays(text):
    """{name: [tile, ...]}, each tile 16 bytes of CGA 2bpp."""
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)
    arrays = {}
    for m in re.finditer(r'unsigned\s+char\s+(\w+)\s*\[[^\]]*\]\s*\[16\]\s*=', text):
        start = text.index('{', m.end())
        depth = 0
        for i in range(start, len(text)):
            depth += {'{': 1, '}': -1}.get(text[i], 0)
            if depth == 0:
                break
        vals = [int(v, 16) for v in re.findall(r'0x[0-9a-fA-F]+', text[start:i + 1])]
        arrays[m.group(1)] = [vals[j:j + 16] for j in range(0, len(vals), 16)]
    return arrays


def cga(tile, cmap):
    """8x8 palette indices from a CGA tile through a colour map."""
    px = []
    for r in range(8):
        row = []
        for b in (tile[2 * r], tile[2 * r + 1]):
            for i in range(4):
                row.append(cmap[(b >> (6 - 2 * i)) & 3])
        px.append(row)
    return px


def topbar(px):
    return [[GOLD] * 8 if r < 2 else list(px[r]) for r in range(8)]


def encode(px):
    """2 bytes of plane truth tables, then 8 rows of (lo, hi) 2bpp."""
    used = sorted({c for row in px for c in row})
    assert len(used) <= 4, used
    code = {c: i for i, c in enumerate(used)}
    planes = []
    for p in range(4):
        t = 0
        for c, i in code.items():
            if (c >> p) & 1:
                t |= 1 << i
        planes.append(t)
    out = [planes[0] | planes[1] << 4, planes[2] | planes[3] << 4]
    for row in px:
        lo = hi = 0
        for x, c in enumerate(row):
            if code[c] & 1:
                lo |= 0x80 >> x
            if code[c] & 2:
                hi |= 0x80 >> x
        out += [lo, hi]
    return out


def main():
    arrays = parse_arrays(open(CHARSET).read())
    tiles = {}          # tile number -> (pixels, character for the tilemap)
    defs = []

    def add(num, px, ch='#'):
        assert num not in tiles and num < 448, hex(num)
        tiles[num] = (px, ch)

    for c in range(0x20, 0x60):
        add(c, cga(arrays['ascii'][c - 0x20], CGA_TEXT), chr(c))

    udg = {}
    for glyph, name, src, idx in UDG:
        udg[name] = glyph
        ch = '#'
        if name.startswith('RANK_'):
            ch = name[-1]
        elif name.startswith('SUIT_'):
            ch = name[5].lower()
        elif name == 'CHIP':
            ch = 'o'
        add(glyph, cga(arrays[src][idx], CGA_ART), ch)

    # Variants, at 0x100 + the glyph they draw.
    add(0x100 | udg['CARD_VERT'], cga(arrays['card_bits'][0], CGA_ART))
    defs.append(('T_CARD_VERT_FACE', 0x100 | udg['CARD_VERT'], 'left border on the card face'))
    red = {0: RED, 1: FELT, 2: RED, 3: FACE}
    for i, r in enumerate(RANKS):
        add(0x100 | (0xAA + i), cga(arrays['black_card_front'][1 + i], red), r)
    defs.append(('T_RANK_RED', 0x1AA, '13 ranks 2..A in red'))
    add(0x1BB, [[FACE] * 8 for _ in range(8)], ' ')
    defs.append(('T_BLANK_FACE', 0x1BB, 'card face interior'))
    add(0x120, topbar(cga(arrays['ascii'][0], CGA_TEXT)), ' ')
    for n in TOPBAR:
        add(0x100 | udg[n], topbar(tiles[udg[n]][0]))
    defs.append(('T_TOPBAR', 0x100, '+ glyph: space and card tops, barred'))

    # drawLine() on the status row: the move under the cursor, black on gold.
    for c in range(0x20, 0x60):
        add(0x140 + c - 0x20, cga(arrays['ascii'][c - 0x20], CGA_HILITE), chr(c))
    defs.append(('T_HILITE', 0x140, '+ glyph - 0x20: ASCII 0x20-0x5F, black on gold'))

    # Runs of consecutive tile numbers.
    data = []
    nums = sorted(tiles)
    i = 0
    while i < len(nums):
        j = i
        while j + 1 < len(nums) and nums[j + 1] == nums[j] + 1 and j - i < 254:
            j += 1
        data += [nums[i] & 0xFF, nums[i] >> 8, j - i + 1]
        for n in nums[i:j + 1]:
            data += encode(tiles[n][0])
        i = j + 1
    data += [0, 0, 0]

    with open(TILES_H, 'w') as f:
        f.write('/* GENERATED by src/sms/mktiles.py from src/msdos/charset.h. Do not edit.\n'
                ' * Glyph codes are what the shadow screen holds; a tile number is the\n'
                ' * glyph itself unless a T_ variant below says otherwise. */\n'
                '#ifndef TILES_H\n#define TILES_H\n\n')
        for glyph, name, _, _ in UDG:
            f.write('#define UDG_%-20s 0x%02X\n' % (name, glyph))
        f.write('\n')
        for name, num, note in defs:
            f.write('#define %-24s 0x%03X  /* %s */\n' % (name, num, note))
        f.write('\n/* Palette indices; see mktiles.py. */\n')
        for n in ['PAPER', 'WHITE', 'RED', 'BLACK', 'FACE', 'FELT', 'GOLD']:
            f.write('#define PAL_%-6s %d\n' % (n, globals()[n]))
        f.write('#define PAL_COUNT  %d\n\n' % len(PALETTE0))
        f.write('extern const unsigned char sms_tileset[];\n'
                'extern const unsigned char sms_palette[];\n\n#endif /* TILES_H */\n')

    with open(TILESET_C, 'w') as f:
        f.write('/* GENERATED by src/sms/mktiles.py from src/msdos/charset.h. Do not edit.\n'
                ' * Runs of { tile lo, tile hi, count }, each tile two bytes of bitplane\n'
                ' * truth tables then eight rows of 2bpp; a zero count ends it. */\n'
                '#ifdef BUILD_SMS\n\n'
                'const unsigned char sms_palette[] = {\n    %s,\n    %s\n};\n\n'
                % (', '.join('0x%02X' % c for c in PALETTE0),
                   ', '.join('0x%02X' % c for c in PALETTE1)))
        f.write('const unsigned char sms_tileset[] = {\n')
        for k in range(0, len(data), 16):
            f.write('    ' + ', '.join('0x%02X' % b for b in data[k:k + 16]) + ',\n')
        f.write('};\n\n#endif /* BUILD_SMS */\n')

    os.makedirs(os.path.dirname(TILEMAP), exist_ok=True)
    with open(TILEMAP, 'w') as f:
        f.write('-- GENERATED by src/sms/mktiles.py: what each tile reads as. "#" is\n'
                '-- card art, a rank is its letter, a suit s/c/d/h, the chip "o".\n'
                'return {\n')
        for n in sorted(tiles):
            ch = tiles[n][1]
            f.write('  [%d] = "%s",\n' % (n, ch.replace('\\', '\\\\').replace('"', '\\"')))
        f.write('}\n')

    print('mktiles: %d tiles, %d bytes' % (len(tiles), len(data)))


if __name__ == '__main__':
    sys.exit(main())
