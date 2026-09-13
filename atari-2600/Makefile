# Makefile -- a convenience wrapper. build.sh is the real build.
#
# The root build (mekkogx) drives the cc65/cmoc-family C toolchains; this port
# is 6502 assembly under Macroassembler AS, which is a different toolchain
# entirely, so it stands alone next to intv/, arcadia/, astrocade/, o2/ and
# channelf/ rather than being forced through machinery it would only
# complicate.

ENDPOINT ?= https://5card.carr-designs.com/
FUJI_FIRMWARE ?= $(HOME)/Workspace/fujinet-firmware
MAME_DIR ?= $(HOME)/Workspace/mame
SECS ?= 200

SRC := $(wildcard src/*.asm src/*.inc)

.PHONY: all layout run runlayout drive smoke resettest shot probe hosttest \
        emu clean

all: build/5card.bin

build/5card.bin: $(SRC) build.sh tools/checkbanks.py tools/mktail.py
	ENDPOINT="$(ENDPOINT)" FUJI_FIRMWARE="$(FUJI_FIRMWARE)" ./build.sh

# The screen with no network at all: the kernel, the colours and the geometry.
layout: build/layout.bin
build/layout.bin: $(SRC) build.sh
	ENDPOINT="$(ENDPOINT)" FUJI_FIRMWARE="$(FUJI_FIRMWARE)" ./build.sh layout

# Interactive, in a window.
run: all
	MAME=$(MAME_DIR) ./run.sh 5card
runlayout: layout
	MAME=$(MAME_DIR) ./run.sh layout

# Headless. `drive` plays a hand against the live server and snapshots it;
# `resettest` proves the cartridge's sequence survives the RESET switch.
#
# NOT -nothrottle. The server's start countdown and its move clock are WALL
# CLOCK, so an unthrottled run sits at "starting in 2" forever and never gets
# a turn. FAST=1 is only for screens that wait on nothing.
drive: all
	MAME=$(MAME_DIR) SECS=$(SECS) ./run.sh 5card drive
smoke: drive
resettest: all
	MAME=$(MAME_DIR) SECS=90 ./run.sh 5card resettest
probe: all
	MAME=$(MAME_DIR) SECS=$(SECS) ./run.sh 5card probe
shot: layout
	MAME=$(MAME_DIR) FAST=1 SECS=8 ./run.sh layout shot

# The cartridge half: FN_BLIT_CARD's art is checked bit-for-bit here and
# nowhere else.
hosttest:
	$(MAKE) -C $(FUJI_FIRMWARE)/pico/atari-2600/firmware/host_test

# Re-graft the cartridge device into MAME. Needed after ANY edit to a shared
# firmware source -- MAME compiles its own copies.
emu:
	$(FUJI_FIRMWARE)/pico/atari-2600/emu/apply.sh
	$(MAKE) -C $(MAME_DIR) -j$$(nproc) NOWERROR=1 REGENIE=1

clean:
	rm -rf build
