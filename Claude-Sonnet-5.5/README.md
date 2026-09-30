# MorphDemo: FROM NOISE TO ORDER

Three real-time audiovisual productions, one per size class, each a complete native Linux executable of exactly
65,536 bytes (`morphdemo-64k`), 16,384 bytes (`morphdemo-16k`) or 4,096 bytes (`morphdemo-4k`). Each production lasts 60 seconds
after a start-up of 0.3 to 1.2 s (compiling the shaders and synthesising the sound), shows the word **MORPH**, ends by itself and stops
cleanly on Escape.

All three tell the same story in their own images: television snow settles into structure and ends as the word. An order
parameter climbs from 0 to 1 during the piece, and picture and sound both read it together with one event schedule (kicks, geiger
clicks, lightning, the drop). That shared schedule is the audio/video synchronisation.

| | picture | sound |
|---|---|---|
| 4k | Snow clears, sparse cubes tumble and lock onto a cubic lattice (cyan), then cell by cell the cubes melt into spheres and the colour turns magenta, the lattice dissolves in a breakdown, and on the drop the letters of MORPH pop up one per beat, made of cubes in front of a receding dot lattice. Raymarched. | GLSL compute synthesis, 120 BPM, A minor. Clicks, drone, kick with sidechain, snare, hats, bass, plucks, pads, riser, one pluck per letter. |
| 16k | Snow tunes out into dust. One million GPU particles condense into a torus, a meridian sphere, a galaxy and a trefoil knot, then fly to the strokes of MORPH while background stars snap onto a crystal lattice. Float target, bloom, depth of field, kick rings. | GLSL compute synthesis plus a convolution-reverb pass, 128 BPM, D minor, 32 bars. Arps with ping-pong echo, pads, lead, bass, drums, riser, drop, one note per letter. |
| 64k | A storm above a cloud sea tunes in out of snow. The camera plunges through the clouds into a valley, a crystallisation front turns the mountains into hexagonal basalt columns, and at sunrise MORPH rises out of the plateau letter by letter. Raymarched landscape with volumetric clouds, lightning, shadows, ambient occlusion, bloom. | Two compute passes (dry mix and send, convolution reverb), 96 BPM, E minor, 24 bars. Storm and thunder, FM bells, strings, pads, formant choir, bass, drums, riser, drop, a plucked Em7 arpeggio with one note per rising letter. |

## Build

`gcc`, `ld`, `objcopy` and `python3` come from the host. `nasm` is the only extra build tool; `shell.nix` declares it.

```
nix-shell --run 'make all'
```

Plain `make all` works as well, also from a bare environment (`env -i PATH=/run/current-system/sw/bin make all`): `tools/nasm.sh` uses
`nasm` from the PATH if there is one and otherwise gets it through `nix-shell shell.nix` (with the system channel when `NIX_PATH` is unset). Single targets: `make morphdemo-4k`,
`make morphdemo-16k`, `make morphdemo-64k`. `make clean` removes `build/` and the three executables. The build is deterministic
(a rebuild from a fresh checkout is byte-identical). The compressor parameters are checked in (`src/*/cm.params`);
`make tune-4k`, `make tune-16k` and `make tune-64k` search new ones after a source change.

## Verification

```
python3 tools/verify.py sizes             # exact byte sizes, ELF magic, executable bit
python3 tools/verify.py run               # real-time run of all three (3 minutes): wall time 50..70 s, exit code 0
make tools && python3 tools/verify.py content   # 10x clock under an LD_PRELOAD hook (needs numpy): shaders link, audio is 60 s and finite,
                                                # the payload unpacked in memory equals the build output byte for byte
make tools && python3 tools/verify.py escape    # injects an Escape key event, expects a clean exit
tools/dev/esctest.sh ./morphdemo-4k       # real Escape key press via uinput into the KDE session (activates the window with kdotool first)
```

Results on the reference host (RTX 5090, KDE Plasma on Wayland, PipeWire, 2026-09-29), see `dev/verification.txt`: all checks pass,
wall time from start to exit is 60.2 s (4k), 60.4 s (16k), 60.5 s (64k) with exit code 0, and Escape ends every production within 0.1 s.

## Implementation

**Executable.** A hand-assembled ELF64 (`tools/stub/stub.asm`): one RWX segment, the interpreter `/lib64/ld-linux-x86-64.so.2`, a
dynamic section that lists `libSDL2-2.0.so.0` and `libGL.so.1` and one relocation for `dlsym`. A 261-byte decoder
(`tools/cm/dec4.inc`) unpacks the payload into the zero-filled tail of the segment and calls it. The payload resolves every SDL
and OpenGL function through `dlsym`. The compressed stream is the last thing in the file, padding before it fixes the exact size.

**Compression.** `tools/cm` is a context-mixing compressor with an integer-only model, identical in the C reference (`cm.h`) and the
assembly decoder: up to 12 hashed contexts, two 8-bit counters per slot, a weighted count ratio as the prediction, an LZMA-style
range coder. `cmtool opt` searches masks, weights and counter limits per payload.

**Payloads.** The 4k payload is assembly (`src/4k/payload.asm`), the 16k and 64k payloads are freestanding C (`src/common/crt.h`,
`src/*/main.c`, `-Os`, flat blob via `tools/payload.ld`). GLSL is minified by `tools/glslmin.py`; the 4k passes one shared
prelude to both of its shaders as the first of two source strings.

**Audio.** The whole 60 seconds are synthesised on the GPU before the first frame: a compute shader evaluates one sample per
invocation from closed-form instruments (kick, snare, saw and FM voices, formant choir, noise). 16k and 64k add a second pass that
convolves a send bus with a decaying noise impulse response. SDL queues the result as 48 kHz float stereo. After one warm-up frame the
audio is unpaused and the visual clock starts, so both share the origin (the audio path adds about 40 ms, measured on a PipeWire sink).

**Graphics.** Everything is procedural. The 4k and 64k are raymarched full-screen fragment shaders (the 64k renders at most 3
megapixels into a float target and lets the post pass scale it), the 16k draws particles from `gl_VertexID`. The drawable size is
queried every frame, so 16:9, ultrawide, portrait and resized windows all show the whole word. The window is fullscreen on the
primary display and the cursor is hidden.

**Development tools** (outside the productions): `tools/dev/gltool.c` renders shader frames and audio without a visible window,
`tools/dev/hook.c` is an `LD_PRELOAD` library that captures frames and audio of the finished executables, dumps the unpacked payload,
injects Escape, scales the clock and reports NaNs, `tools/dev/*.py` analyse the results, and `tools/genword.py` generates the stroke table of the
word MORPH that is pasted into the 16k and 64k shaders. `dev/NOTES.md` holds the environment findings and design notes, `dev/verification.txt` the
final check results and `dev/previews/` one frame per second of each production.
