# MorphDemo — From Noise to Order

Three original 64-second audiovisual productions. Run one directly:

```sh
./morphdemo-64k
./morphdemo-16k
./morphdemo-4k
```

Each opens a 1920×1080 borderless window, synthesizes stereo music, displays
MORPH, fades out and exits automatically. Escape exits early. No arguments,
configuration, production assets, downloads or additional runtime packages
are required. The supplied XWayland display and default audio device are used.

| Executable | Exact bytes | Production |
|---|---:|---|
| `morphdemo-64k` | 65,536 | **Chorus of Matter** — scattered porcelain fragments join into three woven ribbons and a suspended golden crown. |
| `morphdemo-16k` | 16,384 | **Lattice of Light** — drifting luminous frames align into a geometric chamber around nested rotating crystals. |
| `morphdemo-4k` | 4,096 | **Phase Lock** — unstable amber and cyan orbits settle into a fivefold mandala. |

Build from this directory on the supplied machine:

```sh
make -j3 all
make sizes
```

For a clean rebuild:

```sh
make clean
make -j3 all
```

Only the provided GCC, GNU binutils, GNU make and Python 3 are used. The
linker resolves SDL2, OpenGL, libc and libm from
`/run/current-system/sw/share/nix-ld/lib`; native payloads use
`/lib64/ld-linux-x86-64.so.2` and no RPATH. `src/platform.h` declares the small
public ABI subset needed, so separate graphics development headers are
unnecessary.

The C engines synthesize every stereo sample during playback. Their 48 kHz
sample clock drives the GLSL imagery, transitions and 120 BPM visual pulses.
Noise and detuning give way to regular percussion, bells and harmonic pads.
The larger scores add chord progressions and stereo feedback delays. All
geometry, lighting, particles and lettering are procedural. Shaders in
`src/` are embedded at build time; runtime uses no external content files.

The 64 KB executable is a directly linked native ELF. The 16 KB and 4 KB
executables contain their own native payload compressed with the original
LZSS packer in `tools/pack.py`. Their embedded x86-64 code decompresses it,
writes an anonymous `memfd`, and replaces the same process with `execveat`.
There is no child renderer, shell, external decompressor or disk temporary
file. Final files receive zero padding to their exact required lengths.

To repeat the full verification (opens windows and plays all three scores):

```sh
make verify
```

The verifier uses the installed X11 library, PulseAudio monitor, FFmpeg and
strace. Each final ELF completes its sequence from an empty directory, then
passes an Escape test. `verification/report.json` records hashes, exact
sizes, timings, moving frame samples, audio levels and dependency/network
checks. PNGs, WAVs, compressed traces and logs remain in `verification/`;
the demos never load them. The completed instrumented runs took 69.861 s,
67.064 s and 68.487 s respectively, all within 50–70 s. A clean rebuild
produced identical bytes, recorded in `verification/rebuild.json`.
