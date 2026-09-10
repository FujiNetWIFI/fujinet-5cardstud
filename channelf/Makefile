# 5 Card Stud for the Fairchild Channel F.
ASL ?= $(HOME)/asl/asl
P2BIN ?= $(HOME)/asl/p2bin

all: build/5card.bin

build/5card.bin: $(wildcard src/*.asm src/*.inc) tools/mkfont.py
	ASL=$(ASL) P2BIN=$(P2BIN) ./build.sh

clean:
	rm -rf build src/font.inc

.PHONY: all clean
