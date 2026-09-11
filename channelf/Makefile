# 5 Card Stud for the Fairchild Channel F.
ASL ?= $(HOME)/asl/asl
P2BIN ?= $(HOME)/asl/p2bin

all: build/5card.bin

build/5card.bin: $(wildcard src/*.asm src/*.inc) tools/mkfont.py tools/mkcards.py
	ASL=$(ASL) P2BIN=$(P2BIN) ./build.sh

clean:
	rm -rf build src/font.inc src/cardart.inc

.PHONY: all clean
