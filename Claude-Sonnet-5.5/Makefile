# MorphDemo build. Needs: gcc, python3, nasm (through `nix-shell` if it is not installed), see shell.nix.
SHELL := /usr/bin/env bash
.SHELLFLAGS := -euo pipefail -c
CC  ?= gcc
NASM := tools/nasm.sh

all: morphdemo-4k morphdemo-16k morphdemo-64k

tools: build/cmtool build/gltool build/hook.so

build/cmtool: tools/cm/cmtool.c tools/cm/cm.h
	@mkdir -p build build/cm
	$(CC) -O2 -msse4.2 -o $@ tools/cm/cmtool.c -lm

build/gltool: tools/dev/gltool.c
	@mkdir -p build
	$(CC) -O2 -o $@ tools/dev/gltool.c -Wl,--dynamic-linker=/lib64/ld-linux-x86-64.so.2 -L/run/current-system/sw/share/nix-ld/lib -l:libSDL2-2.0.so.0

build/hook.so: tools/dev/hook.c
	@mkdir -p build
	$(CC) -O2 -shared -fPIC -o $@ tools/dev/hook.c -ldl

# ---------------------------------------------------------------- 4k
build/4k/pre.min: src/4k/common.glsl tools/glslmin.py
	@mkdir -p build/4k
	python3 tools/glslmin.py -p src/4k/common.glsl --prelude-only src/4k/vis.glsl > $@
build/4k/vis.min: src/4k/vis.glsl src/4k/common.glsl tools/glslmin.py
	@mkdir -p build/4k
	python3 tools/glslmin.py -p src/4k/common.glsl --body-only src/4k/vis.glsl > $@
build/4k/aud.min: src/4k/aud.comp src/4k/common.glsl tools/glslmin.py
	@mkdir -p build/4k
	python3 tools/glslmin.py -p src/4k/common.glsl --body-only src/4k/aud.comp > $@
build/4k/payload.bin: src/4k/payload.asm build/4k/pre.min build/4k/vis.min build/4k/aud.min
	cd build/4k && ../../$(NASM) -f bin -DPRE_FILE=\"pre.min\" -DVIS_FILE=\"vis.min\" -DAUD_FILE=\"aud.min\" -o payload.bin ../../src/4k/payload.asm
morphdemo-4k: build/4k/payload.bin src/4k/cm.params build/cmtool tools/pack.py tools/stub/stub.asm tools/cm/dec4.inc
	python3 tools/pack.py --payload build/4k/payload.bin --params src/4k/cm.params --libs libSDL2-2.0.so.0,libGL.so.1 --size 4096 --out $@

.PHONY: all tools

# ---------------------------------------------------------------- C payloads (16k, 64k)
CFLAGS_PAYLOAD := -Os -fno-pic -fno-pie -ffreestanding -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables \
  -fno-ident -fcf-protection=none -fno-plt -fomit-frame-pointer -nostdlib -march=x86-64-v3 -fno-math-errno -ffast-math \
  -Wall -Wno-unused-function
DEST16 := 0x410000
BSS16  := 0x5100000

build/16k/pts.vert.min: src/16k/pts.vert src/16k/common.glsl tools/glslmin.py tools/genword.py
	@mkdir -p build/16k
	python3 tools/glslmin.py -p src/16k/common.glsl src/16k/pts.vert > $@
build/16k/pts.frag.min: src/16k/pts.frag tools/glslmin.py
	@mkdir -p build/16k
	python3 tools/glslmin.py src/16k/pts.frag > $@
build/16k/post.frag.min: src/16k/post.frag src/16k/common.glsl tools/glslmin.py
	@mkdir -p build/16k
	python3 tools/glslmin.py -p src/16k/common.glsl src/16k/post.frag > $@
build/16k/aud.min: src/16k/aud.comp src/16k/common.glsl tools/glslmin.py
	@mkdir -p build/16k
	python3 tools/glslmin.py -p src/16k/common.glsl src/16k/aud.comp > $@
build/16k/rev.min: src/16k/rev.comp src/16k/common.glsl tools/glslmin.py
	@mkdir -p build/16k
	python3 tools/glslmin.py -p src/16k/common.glsl src/16k/rev.comp > $@
SHADERS16 := build/16k/pts.vert.min build/16k/pts.frag.min build/16k/post.frag.min build/16k/aud.min build/16k/rev.min
build/16k/main.o: src/16k/main.c src/common/crt.h $(SHADERS16)
	$(CC) -c $(CFLAGS_PAYLOAD) -o $@ src/16k/main.c
build/16k/payload.elf: build/16k/main.o tools/payload.ld
	ld --no-warn-rwx-segments -T tools/payload.ld --defsym=DEST=$(DEST16) --defsym=BSS=$(BSS16) -o $@ build/16k/main.o
build/16k/payload.bin: build/16k/payload.elf
	objcopy -O binary -R .bss $< $@
morphdemo-16k: build/16k/payload.bin src/16k/cm.params build/cmtool tools/pack.py tools/stub/stub.asm tools/cm/dec4.inc
	python3 tools/pack.py --payload build/16k/payload.bin --params src/16k/cm.params --libs libSDL2-2.0.so.0,libGL.so.1 --size 16384 --dest $(DEST16) --memsize 0x4E00000 --out $@
# development build: same source, ordinary dynamic executable
build/16k/host: src/16k/main.c src/common/crt.h $(SHADERS16)
	$(CC) -O1 -DHOST -o $@ src/16k/main.c -Wl,--dynamic-linker=/lib64/ld-linux-x86-64.so.2 -L/run/current-system/sw/share/nix-ld/lib -l:libSDL2-2.0.so.0 -l:libGL.so.1

# ---------------------------------------------------------------- 64k
DEST64 := 0x410000
BSS64  := 0x5100000
build/64k/scene.frag.min: src/64k/scene.frag src/64k/common.glsl tools/glslmin.py tools/genword.py
	@mkdir -p build/64k
	python3 tools/glslmin.py -p src/64k/common.glsl src/64k/scene.frag > $@
build/64k/post.frag.min: src/64k/post.frag src/64k/common.glsl tools/glslmin.py
	@mkdir -p build/64k
	python3 tools/glslmin.py -p src/64k/common.glsl src/64k/post.frag > $@
build/64k/aud.min: src/64k/aud.comp src/64k/common.glsl tools/glslmin.py
	@mkdir -p build/64k
	python3 tools/glslmin.py -p src/64k/common.glsl src/64k/aud.comp > $@
build/64k/rev.min: src/64k/rev.comp src/64k/common.glsl tools/glslmin.py
	@mkdir -p build/64k
	python3 tools/glslmin.py -p src/64k/common.glsl src/64k/rev.comp > $@
SHADERS64 := build/64k/scene.frag.min build/64k/post.frag.min build/64k/aud.min build/64k/rev.min
build/64k/main.o: src/64k/main.c src/common/crt.h $(SHADERS64)
	$(CC) -c $(CFLAGS_PAYLOAD) -o $@ src/64k/main.c
build/64k/payload.elf: build/64k/main.o tools/payload.ld
	ld --no-warn-rwx-segments -T tools/payload.ld --defsym=DEST=$(DEST64) --defsym=BSS=$(BSS64) -o $@ build/64k/main.o
build/64k/payload.bin: build/64k/payload.elf
	objcopy -O binary -R .bss $< $@
morphdemo-64k: build/64k/payload.bin src/64k/cm.params build/cmtool tools/pack.py tools/stub/stub.asm tools/cm/dec4.inc
	python3 tools/pack.py --payload build/64k/payload.bin --params src/64k/cm.params --libs libSDL2-2.0.so.0,libGL.so.1 --size 65536 --dest $(DEST64) --memsize 0x4E00000 --out $@
build/64k/host: src/64k/main.c src/common/crt.h $(SHADERS64)
	$(CC) -O1 -DHOST -o $@ src/64k/main.c -Wl,--dynamic-linker=/lib64/ld-linux-x86-64.so.2 -L/run/current-system/sw/share/nix-ld/lib -l:libSDL2-2.0.so.0 -l:libGL.so.1

# ---------------------------------------------------------------- housekeeping
clean:
	rm -rf build morphdemo-4k morphdemo-16k morphdemo-64k

tune-4k: build/4k/payload.bin build/cmtool
	for s in 1 2 3 4; do cp src/4k/cm.params build/cm/t4_$$s.txt; (build/cmtool opt build/4k/payload.bin build/cm/t4_$$s.txt 12000 $$((s*101)) | tail -1) & done; wait
	best=$$(for s in 1 2 3 4; do echo "$$(build/cmtool cost build/4k/payload.bin build/cm/t4_$$s.txt | sed 's/.*-> \([0-9]*\) .*/\1/') $$s"; done | sort -n | head -1 | cut -d' ' -f2); cp build/cm/t4_$$best.txt src/4k/cm.params
tune-16k: build/16k/payload.bin build/cmtool
	build/cmtool opt build/16k/payload.bin src/16k/cm.params 4000 3
tune-64k: build/64k/payload.bin build/cmtool
	build/cmtool opt build/64k/payload.bin src/64k/cm.params 4000 3

.PHONY: clean tune-4k tune-16k tune-64k
