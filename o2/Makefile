# Makefile -- 5 Card Stud for the Odyssey 2: build/5card.bin (build.sh).
# Standalone, like intv/ and studio2/: not part of the root mekkogx build.
#
#   make                  the image, against the live server
#   make ENDPOINT=http://host:port/
#   make spike            the table engine's test pattern (build/spike)
#   make run              play it in a MAME window (run.sh)
#   make shot             the spike in MAME, headless, and tools/rowcheck.py
#   make rig              build for the private server (test/rig.sh)
#   make test             the live scenarios against the private rig
#   make soak             an AUTOPLAY build plays hands at an AI table

# Every target rebuilds: ENDPOINT and the flags are not files make can see.
ENDPOINT ?= https://5card.carr-designs.com/
RIG_ENDPOINT = http://localhost:8087/

all:
	ENDPOINT='$(ENDPOINT)' ./build.sh

spike:
	SPIKE=1 OUT=build/spike ./build.sh

shot: spike
	O2SIM_SECONDS=20 python3 tools/o2sim.py test/scen/spike.scen build/spike/run build/spike/5card.bin
	python3 tools/rowcheck.py build/spike/run rows

run: all
	./run.sh

rig:
	ENDPOINT='$(RIG_ENDPOINT)' ./build.sh

test: rig
	test/rig.sh test/scen/boot.scen build/run/boot 60

clean:
	rm -rf build

.PHONY: all spike shot run rig test clean
