#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p build/16k/tmp
export TMPDIR="$PWD/build/16k/tmp"
python3 - <<'PY'
import json,re
from pathlib import Path
s=Path('src/16k/scene.frag').read_text()
s=re.sub(r'//[^\n]*','',s)
s='\n'.join(x.strip() for x in s.splitlines() if x.strip())
Path('build/16k/shader.h').write_text('static const char fragment[]='+json.dumps(s)+';\n')
PY
common=(-std=c11 -Os -ffast-math -fno-ident -fno-asynchronous-unwind-tables -fno-stack-protector -Ibuild/16k src/16k/main.c -L/run/current-system/sw/share/nix-ld/lib -l:libSDL2-2.0.so.0 -l:libGL.so.1 -lm -Wl,--dynamic-linker=/lib64/ld-linux-x86-64.so.2,--build-id=none,--hash-style=gnu,-z,noseparate-code,-z,norelro,-z,max-page-size=4096)
gcc "${common[@]}" -s -o build/16k/demo
# nix's compiler wrapper inserts an RPATH; sonames use the supplied nix-ld runtime.
patchelf --remove-rpath build/16k/demo
python3 - <<'PYSH'
import struct
from pathlib import Path
p=Path('build/16k/demo');b=bytearray(p.read_bytes())
phoff=struct.unpack_from('<Q',b,32)[0]
phsize,phnum=struct.unpack_from('<HH',b,54)
end=max(struct.unpack_from('<Q',b,phoff+i*phsize+8)[0]+struct.unpack_from('<Q',b,phoff+i*phsize+32)[0] for i in range(phnum))
struct.pack_into('<Q',b,40,0);struct.pack_into('<HHH',b,58,0,0,0)
p.write_bytes(b[:end])
PYSH
size=$(stat -c %s build/16k/demo)
if ((size>16384));then printf '16k ELF is %s bytes; packing required\n' "$size" >&2;exit 1;fi
cp build/16k/demo morphdemo-16k
truncate -s 16384 morphdemo-16k
chmod +x morphdemo-16k
gcc "${common[@]}" -DDEVELOPMENT -o build/16k/debug
patchelf --remove-rpath build/16k/debug
printf 'morphdemo-16k: %s bytes (ELF payload %s)\n' "$(stat -c %s morphdemo-16k)" "$size"
