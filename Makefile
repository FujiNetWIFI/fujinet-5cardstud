PRODUCT = fcs
#plus4 also works, but needs fujinet-lib
PLATFORMS = apple2 c64 coco
#MSX ROM, MSDOS, and Adam use fujinet-lib-experimental
#Use make-exp <platform> to build them.
#PLATFORMS = msxrom msdos adam

# You can run 'make <platform>' to build for a specific platform,
# or 'make <platform>/<target>' for a platform-specific target.
# Example shortcuts:
#   make coco        → build for coco
#   make apple2/disk → build the 'disk' target for apple2

# SRC_DIRS may use the literal %PLATFORM% token.
# It expands to the chosen PLATFORM plus any of its combos.
SRC_DIRS = src src/%PLATFORM%

# FUJINET_LIB can be
# - a version number such as 4.7.6
# - a directory which contains the libs for each platform
# - a zip file with an archived fujinet-lib
# - a URL to a git repo
# - empty which will use whatever is the latest
# - undefined, no fujinet-lib will be used
FUJINET_LIB =

# Define extra dirs ("combos") that expand with a platform.
# Format: platform+=combo1,combo2
PLATFORM_COMBOS = \
  c64+=commodore \
  atarixe+=atari \
  msxrom+=msx \
  msxdos+=msx \
  adam_cpm+=adam \
  dragon+=coco

CFLAGS_EXTRA_MSDOS = -q -otexan

CFLAGS_EXTRA_MSXROM = -DBUILD_MSX
# Do not add -lmsxbios here: it displaces routines --generic-console relies on
# and the SCREEN 2 display comes up garbled. src/msx/joyread.asm calls the two
# BIOS entries it needs directly instead.
LDFLAGS_EXTRA_MSXROM += --generic-console -pragma-redirect:CRT_FONT=_font -create-app -lm

# z88dk's +coleco -subtype=adam doesn't define an Adam-specific macro, so
# supply the one all of src/adam is guarded by.
CFLAGS_EXTRA_ADAM = -D__ADAM__
LDFLAGS_EXTRA_ADAM += --generic-console -pragma-redirect:CRT_FONT=_font

# ColecoVision cartridge. z88dk puts -D__COLECO__ in the target-wide OPTIONS
# line, so the Adam subtype defines it too -- __COLECO__ alone does not mean
# ColecoVision. src/coleco is therefore guarded by BUILD_COLECO, the same way
# src/msx is, and src/platform-specific/input.h can keep including every
# platform's vars.h unconditionally.
CFLAGS_EXTRA_COLECO = -DBUILD_COLECO
# The console has 1K of RAM and the BIOS owns both ends of it. These two are
# the layout fujinet-firmware/pico/coleco/build.sh proved out: $7000-$702B is
# the cartridge header's own tables, $73B9-$73FF is BIOS scratch, and what is
# left -- 908 bytes -- has to hold BSS, DATA and the C stack. -m leaves a map
# next to the image so that total can actually be read off.
LDFLAGS_EXTRA_COLECO += --generic-console -pragma-redirect:CRT_FONT=_font -m \
  -pragma-define:CRT_ORG_BSS=0x702C \
  -pragma-define:REGISTER_SP=0x73B8 \
  -pragma-define:CLIB_FOPEN_MAX=0

LDFLAGS_EXTRA_APPLE2 = -C src/apple2/apple2-hgr.cfg

# CoCo 3 build: same sources as CoCo 1/2, compiled with -DCOCO3 for the
# standard 320x200x16 GIME mode. Framebuffer lives at $8000 via MMU
# Task 1, so the program can occupy more low memory than the CoCo 1/2
# build, but must still leave room for the C stack at the top of
# $6000-$7FFF (cmoc's default stack lives ~$7Fxx).
COCO_ORG = 1000
ifeq ($(MAKE_COCO3),COCO3)
  CFLAGS_EXTRA_COCO += -DCOCO3
  LDFLAGS_EXTRA_COCO += --org=$(COCO_ORG) --limit=7000
  COCO_CHARSET_SRC := support/coco/pmode3_coco3.fnt
else
  # CoCo 1/2 hires screen lives at $6000 (src/coco/hires.h). cmoc's
  # default org of $2800 would place code/data straight through it, so
  # the program corrupts itself the moment the screen is cleared.
  LDFLAGS_EXTRA_COCO += --org=$(COCO_ORG) --limit=6000
  COCO_CHARSET_SRC := support/coco/pmode3.fnt
endif

## Dragon specific flags (cmoc)
CFLAGS_EXTRA_DRAGON = \
	-Wno-assign-in-condition \
	--no-relocate \
	--intermediate \
	-DDRAGON \
	--dragon
LDFLAGS_EXTRA_DRAGON = --limit=7b00 --org=$(COCO_ORG) --dragon

# Stage the active charset .fnt in build/ so src/coco/charset.s can
# INCLUDEBIN a fixed path; the source it's copied from depends on
# whether MAKE_COCO3 was set. charset.o is rebuilt whenever the source
# .fnt or its destination changes.
build/charset_active.fnt: $(COCO_CHARSET_SRC) | build
	cp $< $@

build:
	mkdir -p $@

build/%/src/coco/charset.o: build/charset_active.fnt

include mekkogx/toplevel-rules.mk

# If you need to add extra platform-specific steps, do it below:
#   coco/r2r:: coco/custom-step1
#   coco/r2r:: coco/custom-step2
# or
#   apple2/disk: apple2/custom-step1 apple2/custom-step2

msdos/disk-post::
	mcopy -t -i $(DISK) src/msdos/AUTOEXEC.BAT "::AUTOEXEC.BAT"

# ColecoVision: headless smoke test in MAME's coleco driver, against a live
# fujinet-pc (the cartridge device dials its BoIP listener on 127.0.0.1:9995).
# MAME resolves rompath, pluginspath and its Lua search path against its OWN
# working directory, so it is run from the MAME tree and everything handed to
# it is absolute -- run it from anywhere else and -autoboot_script is ignored
# silently. That tree needs fujinet-firmware/pico/coleco/emu/apply.sh run
# against it once for -cartslot fujinet to exist.
#
#   make coleco-smoke                        print the screen
#   make coleco-smoke EXPECT="5 CARD STUD"   and assert on it
#   make coleco-smoke SCRIPT="fire,fire"     drive the controller first
#   make coleco-smoke AT=20                  settle longer before sampling
#   make coleco-smoke SETTLE=12 SCRIPT=fire  wait longer before the first press
MAME_DIR    ?= $(HOME)/Workspace/mame
COLECO_ROM  := $(CURDIR)/r2r/coleco/$(PRODUCT).rom
AT          ?= 8
EXPECT      ?=
SCRIPT      ?=
# Each scripted press costs a hold plus a gap; 3s of settle before the first.
SETTLE      ?= 8
SECS        ?= $(shell echo $$(( $(AT) + $(SETTLE) + 2 + $(words $(subst $(comma), ,$(SCRIPT))) )))
comma       := ,

.PHONY: coleco-smoke

coleco-smoke: $(COLECO_ROM)
	cd $(MAME_DIR) && \
	FCS_FONT=$(CURDIR)/src/coleco/font.bin FCS_AT=$(AT) FCS_EXPECT="$(EXPECT)" \
	FCS_SCRIPT="$(SCRIPT)" FCS_SETTLE=$(SETTLE) \
	./mame coleco -cartslot fujinet -cart $(COLECO_ROM) \
	    -video none -sound none -nothrottle -seconds_to_run $(SECS) \
	    -autoboot_script $(CURDIR)/support/coleco/smoke.lua

# CoCo targets:
#   make coco        → CoCo 1/2 build
#   make coco3       → CoCo 3 build (40-column hires layout)
#   make coco-dist   → combined disk with loader + both CoCo binaries

.PHONY: coco3 coco-dist

coco3:
	$(MAKE) coco MAKE_COCO3=COCO3

# Combined CoCo 1/2 + CoCo 3 disk. The loader (support/coco/loader.c)
# auto-detects the model and runs FCS12 or FCS3.
R2R_PRODUCT = r2r/coco/$(PRODUCT)
COCO_DISK   = $(R2R_PRODUCT).dsk

coco-dist:
	$(MAKE) clean
	rm -rf build
	$(MAKE) coco
	mv $(R2R_PRODUCT).bin $(R2R_PRODUCT)12.bin

	rm -rf build
	$(MAKE) coco3
	mv $(R2R_PRODUCT).bin $(R2R_PRODUCT)3.bin

	cmoc -o $(R2R_PRODUCT).bin support/coco/loader.c

	$(RM) $(COCO_DISK)
	decb dskini $(COCO_DISK)
	decb copy -t -0 support/coco/autoexec.bas $(COCO_DISK),AUTOEXEC.BAS
	decb copy -b -2 $(R2R_PRODUCT).bin   $(COCO_DISK),FCS.BIN
	decb copy -b -2 $(R2R_PRODUCT)12.bin $(COCO_DISK),FCS12.BIN
	decb copy -b -2 $(R2R_PRODUCT)3.bin  $(COCO_DISK),FCS3.BIN
