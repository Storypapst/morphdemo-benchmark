# Environment probes

Small C programs that answered questions about the host before the demos were designed. Build any of them with

```
gcc -O1 -o name name.c -Wl,--dynamic-linker=/lib64/ld-linux-x86-64.so.2 -L/run/current-system/sw/share/nix-ld/lib -l:libSDL2-2.0.so.0
```

| file | question |
|---|---|
| `probe.c` | Which video and audio driver does SDL pick, which GL version comes up, what size does a fullscreen desktop window get, does vsync pace the loop? |
| `lat.c` | How is the SDL audio queue consumed (chunk size, time until the first chunk)? |
| `latprobe.c`, `latmeasure.py` | Latency from `SDL_PauseAudioDevice` to a click in the monitor of a PipeWire sink (`latmeasure.py` expects the binary at `build/latprobe` and records the loopback sink). |
| `focus.c` | Does a fullscreen window receive keyboard focus by itself, and does the window flag path (windowed first, fullscreen later) change that? |
