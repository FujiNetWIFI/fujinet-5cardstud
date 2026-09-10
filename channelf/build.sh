#!/usr/bin/env bash
# build.sh -- assemble 5 Card Stud for the Fairchild Channel F.
#
# Macro Assembler AS with CPU F3850, the same assembler the Arcadia (2650) and
# O2 (8048) ports use. The image must be exactly 16K with the "FUJI" claim at
# $47FC, or the cartridge boots it with the mailbox dead.
set -euo pipefail
cd "$(dirname "$0")"

ASL="${ASL:-$HOME/asl/asl}"
P2BIN="${P2BIN:-$HOME/asl/p2bin}"

[ -x "$ASL" ]   || { echo "build.sh: no assembler at $ASL" >&2; exit 1; }
[ -x "$P2BIN" ] || { echo "build.sh: no p2bin at $P2BIN" >&2; exit 1; }

mkdir -p build
python3 tools/mkfont.py src/font.inc

( cd src && "$ASL" 5card.asm -L -i . -q )
"$P2BIN" src/5card.p build/5card.bin -r 0x800-0x47ff -l 0xff -q
mv -f src/5card.lst build/5card.lst 2>/dev/null || true
rm -f src/5card.p
python3 tools/checkrom.py build/5card.bin --claim
