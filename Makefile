PRODUCT = fcs
#TODO FIX adam
#plus4 also works, but needs fujinet-lib
PLATFORMS = apple2 c64 coco
#MSX ROM and MSDOS use fujinet-lib-experimental
#Use make-exp <platform> to build them.
#PLATFORMS = msxrom msdos

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
  adam_cpm+=adam

CFLAGS_EXTRA_MSDOS = -q -otexan

CFLAGS_EXTRA_MSXROM = -DBUILD_MSX
LDFLAGS_EXTRA_MSXROM += --generic-console -pragma-redirect:CRT_FONT=_font -create-app -lm

LDFLAGS_EXTRA_APPLE2 = -C src/apple2/apple2-hgr.cfg

# CoCo 3 build: same sources as CoCo 1/2, compiled with -DCOCO3 for the
# standard 320x200x16 GIME mode. Framebuffer lives at $8000 via MMU
# Task 1, so the program can occupy more low memory than the CoCo 1/2
# build, but must still leave room for the C stack at the top of
# $6000-$7FFF (cmoc's default stack lives ~$7Fxx).
ifeq ($(MAKE_COCO3),COCO3)
  CFLAGS_EXTRA_COCO += -DCOCO3
  LDFLAGS_EXTRA_COCO += --org=1000 --limit=7000
  COCO_CHARSET_SRC := support/coco/pmode3_coco3.fnt
else
  # CoCo 1/2 hires screen lives at $6000 (src/coco/hires.h). cmoc's
  # default org of $2800 would place code/data straight through it, so
  # the program corrupts itself the moment the screen is cleared.
  LDFLAGS_EXTRA_COCO += --org=1000 --limit=6000
  COCO_CHARSET_SRC := support/coco/pmode3.fnt
endif

# Stage the active charset .fnt in build/ so src/coco/charset.s can
# INCLUDEBIN a fixed path; the source it's copied from depends on
# whether MAKE_COCO3 was set. charset.o is rebuilt whenever the source
# .fnt or its destination changes.
build/charset_active.fnt: $(COCO_CHARSET_SRC) | build
	cp $< $@

build:
	mkdir -p $@

build/coco/src/coco/charset.o: build/charset_active.fnt

include mekkogx/toplevel-rules.mk

# If you need to add extra platform-specific steps, do it below:
#   coco/r2r:: coco/custom-step1
#   coco/r2r:: coco/custom-step2
# or
#   apple2/disk: apple2/custom-step1 apple2/custom-step2

msdos/disk-post::
	mcopy -t -i $(DISK) src/msdos/AUTOEXEC.BAT "::AUTOEXEC.BAT"

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
