#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p build/4k
python3 - <<'PY'
import json,pathlib,re
s=pathlib.Path('src/4k/scene.frag').read_text()
first,rest=s.split('\n',1)
rest=re.sub(r'\s+',' ',rest)
rest=re.sub(r'\s*([{}();,+*/=<>?:&|\-])\s*',r'\1',rest)
pathlib.Path('build/4k/shader.h').write_text(json.dumps(first+'\n'+rest))
PY
gcc -Os -ffast-math -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident -fno-pie -no-pie -nostartfiles -Ibuild/4k src/4k/demo.c -o build/4k/payload -Wl,--build-id=none,--hash-style=gnu,-z,norelro,-z,noseparate-code,-s,--dynamic-linker=/lib64/ld-linux-x86-64.so.2 -L/run/current-system/sw/share/nix-ld/lib -l:libSDL2-2.0.so.0 -l:libGL.so.1 -l:libm.so.6 -l:libc.so.6
patchelf --remove-rpath build/4k/payload
python3 src/4k/pack.py
