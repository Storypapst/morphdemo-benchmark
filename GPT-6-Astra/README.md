# MORPH — From Noise to Order

Three original, self-contained, one-minute audiovisual productions for the machine described in `ENVIRONMENT.json`.

| Executable | Exact size | Production |
|---|---:|---|
| `morphdemo-64k` | 65,536 bytes | **The Loom** — wandering filaments become an intricate luminous, woven celestial structure. Layered synthesis, etched surfaces, atmospheric dust and bloom. |
| `morphdemo-16k` | 16,384 bytes | **Crystal Assembly** — scattered mineral fragments gather into a faceted crystal and ordered golden orbits. Procedural reflections, stereo synthesis and rhythmic light. |
| `morphdemo-4k` | 4,096 bytes | **Phase Lock** — unstable wave fragments acquire rhythm and resolve into three resonant rings. A compact synthesized score shares the visual pulse. |

Each runs for approximately 60 seconds, displays **MORPH**, fades out and exits automatically. **Escape** exits early. Run one at a time:

```sh
./morphdemo-64k
./morphdemo-16k
./morphdemo-4k
```

## Build

From this directory, using the GCC, binutils, Python 3 and patchelf already supplied on this machine:

```sh
bash build.sh
```

This rebuilds all three and checks their ELF signatures, executable permissions and exact sizes. Individual builds are also supported:

```sh
bash src/64k/build.sh
bash src/16k/build.sh
bash src/4k/build.sh
```

Builds require no downloads. Temporary build products are kept in `build/`. The programs use the installed SDL2, OpenGL, libc and libm libraries, resolved through the supplied `/lib64/ld-linux-x86-64.so.2` loader and nix-ld library environment.

## Implementation

All scene geometry, surfaces, lettering, animation, instruments and music are calculated from embedded code and parameters. GLSL files in `src/` are embedded during compilation; the executables do not load those files at runtime. No production assets, external fonts, prerecorded audio or network access are used. Each executable can be copied and run independently.

The 16K and 64K programs are directly linked native ELF files. The 4K program contains a native x86-64 LZSS decoder and its own compressed ELF payload. It decompresses into memory, writes that payload to an anonymous `memfd`, and executes it with `execveat`. It invokes no shell, external unpacker, or other demo. The packer checks a decompression round trip during the build. Padding supplies the exact final file sizes.

The 64K executable additionally supports generated still and score exports:

```sh
./morphdemo-64k --time 40 --capture build/loom.ppm
./morphdemo-64k --audio build/loom.wav
```

## Verification

```sh
python3 tools/verify.py
python3 tools/verify.py --escape
```

These run the actual final binaries sequentially from otherwise empty directories. A development-only SDL/OpenGL interposer captures frames and synthesized samples, records timing and frame rate, checks non-silent unclipped audio, and injects Escape for the second command. `strace` records runtime file access, process execution and socket calls. Tests enforce the 50–70 second full-run range, exact file sizes, clean exits and absence of Internet sockets or delegated renderers. No verification code is linked into the productions.

Captured frames, audio, syscall logs and machine-readable results are in `verification/`; `verification/RESULTS.md` records the final checks. Pillow, when available, creates PNG contact sheets; it is optional and is never a runtime dependency of the demos.
