#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p build/64k
python3 - <<'PY'
import json,pathlib
base=pathlib.Path('src/64k')
out=pathlib.Path('build/64k/shaders.h')
out.write_text('\n'.join('static const char *'+name+'_shader =\n'+json.dumps((base/(name+'.frag')).read_text())+';\n' for name in ('scene','post')))
PY
cc -std=c99 -O2 -ffast-math -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident -Ibuild/64k src/64k/main.c -o build/64k/loom.unpadded -L/run/current-system/sw/share/nix-ld/lib -Wl,--dynamic-linker=/lib64/ld-linux-x86-64.so.2 -Wl,--build-id=none -Wl,-z,noseparate-code -l:libSDL2-2.0.so.0 -l:libGL.so.1 -lm
patchelf --remove-rpath build/64k/loom.unpadded
strip --strip-all build/64k/loom.unpadded
python3 - <<'PY'
import pathlib
p=pathlib.Path('build/64k/loom.unpadded').read_bytes()
assert p[:4]==b'\x7fELF'
assert len(p)<=65536, f'64k executable too large: {len(p)}'
out=pathlib.Path('morphdemo-64k');out.write_bytes(p+bytes(65536-len(p)));out.chmod(0o755)
print(f'morphdemo-64k: {len(p)} native bytes, padded to {out.stat().st_size}')
PY
