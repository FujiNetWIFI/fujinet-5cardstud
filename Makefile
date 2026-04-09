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

include mekkogx/toplevel-rules.mk

# If you need to add extra platform-specific steps, do it below:
#   coco/r2r:: coco/custom-step1
#   coco/r2r:: coco/custom-step2
# or
#   apple2/disk: apple2/custom-step1 apple2/custom-step2

msdos/disk-post::
	mcopy -t -i $(DISK) src/msdos/AUTOEXEC.BAT "::AUTOEXEC.BAT"
