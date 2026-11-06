#!/usr/bin/env python3
"""player.py -- a second human for the rig: plays 5 Card Stud over HTTP.

    player.py [--server URL] [--table ID] [--name NAME] [--delay S]
              [--hands N]

Joins TABLE as NAME and, on its turn, DELAY seconds in, checks or calls if
it may (else the first move offered). Reads the binary state, the same bytes
the O2 does (src/equ.inc). Prints a line per change of round or turn; leaves
after N hands.
"""
import argparse
import sys
import time
import urllib.request


def get(server, path, table, name):
    url = "%s%s?table=%s&player=%s&bin=1" % (server, path, table, name)
    with urllib.request.urlopen(url, timeout=5) as r:
        return r.read()


def cstr(b):
    return b.split(b"\0")[0].decode()


def parse(b):
    moves = [(cstr(b[88 + 13 * i:91 + 13 * i]), cstr(b[91 + 13 * i:101 + 13 * i]))
             for i in range(min(b[87], 5))]
    n = b[153]
    seats = [{"name": cstr(b[154 + 33 * k:163 + 33 * k]), "hand": cstr(b[176 + 33 * k:187 + 33 * k])}
             for k in range(n)]
    return {"result": cstr(b[0:81]), "round": b[81], "active": b[84], "moves": moves, "seats": seats}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--server", default="http://localhost:8087/")
    ap.add_argument("--table", default="basement")
    ap.add_argument("--name", default="py")
    ap.add_argument("--delay", type=float, default=1.5)
    ap.add_argument("--hands", type=int, default=3)
    a = ap.parse_args()
    seen, turn_at, hands, last_round = None, None, 0, None
    while True:
        s = parse(get(a.server, "state", a.table, a.name))
        key = (s["round"], s["active"])
        if key != seen:
            print("round %d active %d %s %s" % (s["round"], s["active"],
                  [x["name"] for x in s["seats"]], s["result"]), flush=True)
            seen, turn_at = key, time.time()
        if last_round == 5 and s["round"] != 5:
            hands += 1
            if hands >= a.hands:
                get(a.server, "leave", a.table, a.name)
                print("left", flush=True)
                return
        last_round = s["round"]
        if s["active"] == 0 and s["moves"] and time.time() - turn_at >= a.delay:
            codes = [m[0] for m in s["moves"]]
            pick = next((c for c in ("ch", "ca", "bb") if c in codes), codes[0])
            print("  move %s" % pick, flush=True)
            get(a.server, "move/" + pick, a.table, a.name)
            seen = None
        time.sleep(0.5)


if __name__ == "__main__":
    sys.exit(main())
