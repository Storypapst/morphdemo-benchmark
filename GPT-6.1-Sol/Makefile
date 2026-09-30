SHELL := /run/current-system/sw/bin/bash
.SHELLFLAGS := -euo pipefail -c
CC := gcc
LD := ld
LIBDIR := /run/current-system/sw/share/nix-ld/lib
CFLAGS := -Os -ffast-math -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident -ffunction-sections -fdata-sections -msse4.1 -Wall -Wextra
LDFLAGS := -m elf_x86_64 -dynamic-linker /lib64/ld-linux-x86-64.so.2 -z noseparate-code -z norelro --hash-style=sysv --build-id=none --gc-sections -s -e _start
LIBS := -L$(LIBDIR) -lSDL2 -lGL -lm -lc
export TMPDIR := $(CURDIR)/build/tmp
export PYTHONDONTWRITEBYTECODE := 1

.PHONY: all clean verify sizes
all: morphdemo-64k morphdemo-16k morphdemo-4k

build/tmp:
	mkdir -p $@

build/entry.o: src/entry.S | build/tmp
	mkdir -p build
	$(CC) -c $< -o $@

build/64/shader.h: src/common.glsl src/chorus.glsl tools/embed.py
	python3 tools/embed.py $@ src/common.glsl src/chorus.glsl
build/16/shader.h: src/common.glsl src/lattice.glsl tools/embed.py
	python3 tools/embed.py $@ src/common.glsl src/lattice.glsl
build/4/shader.h: src/phase.glsl tools/embed.py
	python3 tools/embed.py $@ src/phase.glsl

build/64.o: src/demo.c src/music.h src/platform.h build/64/shader.h | build/tmp
	$(CC) $(CFLAGS) -DDEMO=64 -Ibuild/64 -c $< -o $@
build/16.o: src/demo.c src/music.h src/platform.h build/16/shader.h | build/tmp
	$(CC) $(CFLAGS) -DDEMO=16 -Ibuild/16 -c $< -o $@
build/4.o: src/tiny.c src/platform.h build/4/shader.h | build/tmp
	$(CC) $(CFLAGS) -Ibuild/4 -c $< -o $@

build/%.elf: build/%.o build/entry.o | build/tmp
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

morphdemo-64k: build/64.elf tools/pack.py
	python3 tools/pack.py $< $@ 65536
morphdemo-16k: build/16.elf tools/pack.py src/unpack.S
	python3 tools/pack.py $< $@ 16384
morphdemo-4k: build/4.elf tools/pack.py src/unpack.S
	python3 tools/pack.py $< $@ 4096

sizes: all
	python3 -c 'from pathlib import Path; expected={"morphdemo-64k":65536,"morphdemo-16k":16384,"morphdemo-4k":4096}; actual={p:Path(p).stat().st_size for p in expected}; assert actual==expected,actual; [print(p,n) for p,n in actual.items()]'

verify: all
	python3 tools/verify.py

clean:
	rm -rf build morphdemo-64k morphdemo-16k morphdemo-4k

.SECONDARY:
