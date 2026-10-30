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
#   make eight            ... at dev7, eight seats (DEVTABLES lists it)
#   make two              ... against test/player.py, two humans

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
	test/rig.sh test/scen/play.scen build/run/play 50
	NAME= test/rig.sh test/scen/name.scen build/run/name 25

soak:
	AUTOPLAY=1 OUT=build/auto ENDPOINT='$(RIG_ENDPOINT)' ./build.sh
	O2SIM_SHOTS=300 test/rig.sh test/scen/autoplay.scen build/run/auto 170 build/auto/5card.bin

eight:
	AUTOPLAY=1 DEVTABLES=1 OUT=build/dev ENDPOINT='$(RIG_ENDPOINT)' ./build.sh
	test/rig.sh test/scen/eight.scen build/run/eight 40 build/dev/5card.bin

two:
	AUTOPLAY=1 OUT=build/auto ENDPOINT='$(RIG_ENDPOINT)' ./build.sh
	PLAYER="--table basement --hands 3" O2SIM_SHOTS=600 test/rig.sh test/scen/twohuman.scen build/run/two 110 build/auto/5card.bin

clean:
	rm -rf build

.PHONY: all spike shot run rig test soak eight two clean
