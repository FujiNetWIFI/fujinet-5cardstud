#!/usr/bin/env bash
# Headless smoke test for the H89 build, in FujiNet Go H89's session runner
# (h89_session_run: the emulated H89 and H19, with the in-process FujiNet on
# its card). Boots 64K CP/M 2.2.04 or HDOS 2.0 with the game's disk in drive
# 2, starts the game, waits for EXPECT on screen, runs any further session
# runner steps, and prints the screen as text (H19 graphics as Unicode).
#
#   support/h89/smoke.sh cpm|hdos DISK.h8d [STEP...]
#
# Environment: PROGRAM (the program's name on the disk, default FCS), H89_EMU
# (the FujiNet Go H89 tree), EXPECT (default the title screen's
# "F U J I N E T"), H89_HOME (where the disks and FujiNet's SD and appkeys
# live between runs), TIMEOUT (seconds), SHOT (a .ppm to save the final
# screen to). The runner's --type drops control characters, so ESC is
# --key f1, which the game takes as ESC.
set -uo pipefail

OS="$1"
DISK="$2"
shift 2

EMU="${H89_EMU:-$HOME/Workspace/fujinet-go-h89-desktop}"
HOME_DIR="${H89_HOME:-$(pwd)/build/h89-smoke}"
EXPECT="${EXPECT:-F U J I N E T}"
PROGRAM="${PROGRAM:-FCS}"
RUN="$EMU/build/core/h89_session_run"

for f in "$RUN" "$DISK"; do
    [[ -e "$f" ]] || { echo "missing: $f" >&2; exit 77; }
done

mkdir -p "$HOME_DIR"
cp "$DISK" "$HOME_DIR/game.h8d"

case "$OS" in
cpm)
    cp "$EMU/tools/disks/boot/cpm-2.2.04-64k.h8d" "$HOME_DIR/os.h8d"
    BOOT=(--until 'H:' --type 'B\r' --until 'A>' --wait 1000
          --prompt 'A>' --type "B:$PROGRAM\r")
    ;;
hdos)
    cp "$EMU/tools/disks/boot/hdos-2.0.h17disk" "$HOME_DIR/os.h8d"
    BOOT=(--until 'H:' --type 'B\r' --wait 3000 --type ' ' --wait 500 --type ' '
          --prompt 'ACTION? <BOOT>' --type '\r'
          --prompt 'DATE (DD-MMM-YY)?' --type '11-OCT-80\r'
          --prompt '>' --type 'MOUNT SY1:\r' --wait 3000
          --prompt '>' --type "RUN SY1:$PROGRAM.ABS\r")
    ;;
*)
    echo "usage: $0 cpm|hdos DISK.h8d [STEP...]" >&2
    exit 2
    ;;
esac

SHOT_STEP=()
[[ -n "${SHOT:-}" ]] && SHOT_STEP=(--ppm "$SHOT")

"$RUN" --roms "$EMU/tools/roms" --home "$HOME_DIR/home" --timeout "${TIMEOUT:-300}" \
    --disk "h17:1=$HOME_DIR/os.h8d" --disk "h17:2=$HOME_DIR/game.h8d" \
    "${BOOT[@]}" --until "$EXPECT" "$@" --screen "${SHOT_STEP[@]}" \
    > "$HOME_DIR/run.log" 2>&1
rc=$?

# The runner's log escapes non-ASCII bytes and mixes in FujiNet's own log
python3 -I -c '
import re, sys
show = False
for line in open(sys.argv[1], encoding="latin-1"):
    line = re.sub(r"^\[fujinet\] ", "", line.rstrip("\n"))
    if line.startswith("---- screen"):
        show = True
    if show:
        b = re.sub(r"\\x([0-9A-Fa-f]{2})", lambda m: chr(int(m.group(1), 16)), line)
        print(b.encode("latin-1").decode("utf-8", "replace"))
    if line.startswith("----------------"):
        show = False
' "$HOME_DIR/run.log"

[[ $rc -eq 0 ]] || echo "h89-smoke: failed (rc=$rc); see $HOME_DIR/run.log" >&2
exit $rc
