# MorphDemo working notes

Development notes for the three productions. They sit outside the deliverables and record what was measured on the reference host
(NixOS 26.05, RTX 5090, KDE Plasma on Wayland, PipeWire) on 2026-09-29, the design of each demo and the mistakes worth avoiding.

## Environment facts

- SDL2 on this host is sdl2-compat 2.32.68 running on SDL3. Video driver wayland (x11 works too through `SDL_VIDEODRIVER=x11`), audio driver pipewire, GL 4.6 compatibility context on the RTX 5090.
- The ELF interpreter `/lib64/ld-linux-x86-64.so.2` is nix-ld 2.0.6 (Rust). It maps the real glibc 2.42 `ld.so` itself and jumps into it, so the kernel is the only reader of the ELF header.
  Libraries resolve from `/run/current-system/sw/share/nix-ld/lib` without RUNPATH.
- Hand-made ELF and glibc: `DT_RELAENT` (tag 9) is mandatory whenever `DT_RELA` is present (NULL dereference otherwise). No `PT_GNU_STACK`, hash table or version info is needed.
- `dlsym(RTLD_DEFAULT, ...)` finds the symbols of the `DT_NEEDED` libraries (`libSDL2-2.0.so.0`, `libGL.so.1`) and nothing else. `libGL.so.1` exports all GL 4.6 functions including `glRects`, so every GL entry point comes from `dlsym`.
- The fullscreen window lands on the primary output and takes its native pixel size: 3840x2560 (3:2) on DP-6 here, 2160x3840 on a rotated output, 3840x2160 elsewhere.
  All shaders therefore frame with `asp=min(h,w/1.7778)` and query the drawable size every frame.
- Frame pacing follows the output the window lands on. HDMI-A-2 currently runs 3840x2160 at 30 Hz, so windowed test runs there show 30 fps.
- The audio queue starts to be consumed about 7 ms after `SDL_PauseAudioDevice`. The path SDL to a PipeWire sink monitor adds 36 to 44 ms for SDL buffer sizes 0, 256, 512 and 1024 (`dev/probe/latprobe.c` with `latmeasure.py`). No compensation is applied.
- Keyboard focus: in an idle session the fullscreen window receives keyboard focus by itself (`dev/probe/focus.c`, 12 of 12 runs). While the user types in another window, KWin focus stealing prevention hands the focus back within a millisecond.
  `tools/dev/esctest.sh` sends a real Escape key press through uinput and activates the window first with kdotool unless `NOACTIVATE=1` is set.
- float32 `sin(2*pi*fract(f*t))` loses precision for large `t`: oscillator times stay local (per bar or per note) in every score.

## Toolchain (`tools/`)

| file | purpose |
|---|---|
| `glslmin.py` | GLSL minifier: renames identifiers, prelude names stay identical between shaders (`--prelude-only`, `--body-only` split the output for the 4k). |
| `cm/cm.h`, `cm/cmtool.c` | cm4 compressor: integer count mixing, 32-bit slots, crc32c hashing, LZMA-style range coder. `cmtool enc`, `dec`, `cost`, `opt`, `prof`. |
| `cm/cmexp.c` | floating-point model experiments (logistic mixing, dual-rate counters, count mixing) that decided the model family; not part of the build. |
| `cm/dec4.inc` | the x86-64 decoder (261 bytes); `cm/dectest.sh` decodes a stream with it in a test ELF and compares. |
| `stub/stub.asm` | the hand-made ELF: PT_INTERP, one RWX PT_LOAD, PT_DYNAMIC with the needed libraries and one relocation (dlsym lands in the `e_shoff` field at 0x400028). |
| `pack.py` | payload to cm stream to ELF of an exact size; the stream is last, padding goes before it. |
| `payload.ld` | linker script that turns the freestanding C payloads into one flat blob. |
| `genword.py` | emits the GLSL table of line segments for the word MORPH (pasted into `src/16k/pts.vert` and `src/64k/scene.frag`). |
| `verify.py` | acceptance checks (sizes, run, content, escape). |
| `nasm.sh` | nasm from the PATH or through nix-shell. |
| `dev/gltool.c` | renders fragment shaders to PPM, benchmarks them, runs compute-shader audio (one and two pass) without a visible window. |
| `dev/hook.c` | LD_PRELOAD hook for the finished executables: frame capture, audio capture, Escape injection, clock speed and speed schedule, payload dump, shader link log, NaN scan, window size override. |
| `dev/*.py`, `dev/*.sh` | analysis: spectrogram and levels (`wavstat.py`), BS.1770 loudness and octave balance (`loudness.py`), pitch classes per bar (`chroma.py`), layer solo (`solo.py`), frame pacing and jumps (`framestat.py`, `pops.py`), contact sheets (`sheet.py`, `shots.py`, `strip.py`, `crop.py`), filmstrips (`film.sh`), end-to-end audio path test (`audiotest.py`), real key press test (`esctest.sh`). |

Typical loop for a visual change: edit the shader, `./tools/dev/frames.sh` on the `mkshader.py` output for a quick contact sheet, `make morphdemo-xk`, `tools/dev/film.sh xk tag` for a filmstrip of the real executable
(`SPEEDS="0:8,44:1,49:8"` runs part of it in real time), then `python3 tools/verify.py content`. After a source change run `make tune-xk` and commit the new `cm.params`.

## Design notes

**Shared schedule.** `common.glsl` of every demo holds the order curve `ORD(t)`, the static level `STA(t)`, the kick schedule (`KT` jittered start times, `KV` which kicks sound, `KE` the envelope the picture uses),
the geiger clicks `CK`, and in the 64k the lightning `LG` with its thunder `TH` and the kick age `KP`. Audio and picture include the same file, so a pulse in the picture is the same event as the sound.

**4k (4096 bytes, assembly payload).** One fragment shader (cube lattice, morph, dot-matrix word) and one compute shader (score), both minified and sharing a 744-byte prelude that is passed as the first source string of
`glCreateShaderProgramv`. The payload is 8194 bytes unpacked at 0x401000 and must end below 0x440000, where the import table lives (`pack.py` checks it). About 90 bytes of the file are padding.
Timeline: snow 0..3 s, sparse cubes, order 8..30 s, cubes melt into spheres 26..41 s (magenta phase 28..40), breakdown 41..44 with a snare roll, breath of silence 43.7, drop and letters 44..46, word to 58, fade 58..60.
Bass register is 55 to 110 Hz on purpose: earlier versions put the bass at 27 to 55 Hz where small speakers lose it.

**16k (16384 bytes, C payload).** `pts.vert` computes 1.35 million points from `gl_VertexID` alone: 1,048,576 shape particles (dust, torus, meridian sphere, galaxy, trefoil knot, letter strokes, blended by a per-particle
phase so shapes dissolve into each other), 100,000 background stars that settle onto a lattice in the last quarter, 200,000 particles in five kick rings. Rendering goes into an RGBA16F target that follows the drawable size;
`post.frag` adds mip-chain bloom, chromatic aberration, vignette, grain and the snow at the start.

**64k (65536 bytes, C payload).** `scene.frag` is one raymarched scene: winding valley of ridged noise, hex-prism columns that a crystallisation front (`FR`) switches on, height field word `wordh()` that rises letter by letter,
volumetric cloud deck with lightning, sky with sun and stars, soft shadows, ambient occlusion, a short reflection march for the finale. The scene renders into a float target of at most 3 megapixels
(rebuilt when the window size changes), `post.frag` scales it to the window with bloom, grade and snow.

## Mistakes worth avoiding

- Minified GLSL renames identifiers per shader: varyings between vertex and fragment stage need `layout(location=N)`.
- Compatibility profile: `gl_PointCoord` needs `glEnable(GL_POINT_SPRITE)`; `glDrawArrays` with only `gl_VertexID` needs vertex attribute 0 enabled (point it at any buffer).
- `exp(-x)` with a very negative argument overflows to inf and `0*inf` gives NaN; `pow(negative, 2.)` gives NaN. One NaN pixel in the HDR target spreads through the mip chain as a black block.
  `HOOK_NAN=k` scans the float target of a running demo.
- Multi-pass compute needs `glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)` between dispatches, and the mapping of the result needs `GL_BUFFER_UPDATE_BARRIER_BIT`.
- The stream of the compressed payload is the last thing in the file and the decoder reads a few bytes past it (in the 4k those bytes are the start of the unpacked payload). The range coder flush in `cm.h` keeps the
  interval valid for any trailing bytes; `cmtool enc` proves it with zero, 0xff and random trailers.
- `tools/dev/audcheck.sh` writes `aud_dev.min`; the Makefile owns `aud.min`.
- Harmony helpers must all read the same chord function (a stray `th()` call made a pad play G minor in the 64k; `solo.py` found it).
- Look at frames at native resolution before calling something finished: two defects of the 64k (a double-counted sky reflection and an over-long shadow ray that dimmed a whole patch of the floor) were invisible in thumbnails.

## Results

`dev/verification.txt` holds the final checks; `dev/previews/` has one frame per second of each demo. Gate list for a release: `make all`, `python3 tools/verify.py sizes content escape run`, `python3 tools/dev/audiotest.py 4k|16k|64k`,
`NOACTIVATE=1 tools/dev/esctest.sh ./morphdemo-4k`, a build from a copy without `build/` compared byte for byte.
