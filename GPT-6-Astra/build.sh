#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
for size in 4k 16k 64k; do
    bash "src/$size/build.sh"
done
python3 - <<'PY'
from pathlib import Path
for size, expected in [('4k',4096),('16k',16384),('64k',65536)]:
    p=Path('morphdemo-'+size)
    assert p.read_bytes()[:4]==b'\x7fELF', f'{p}: missing ELF signature'
    assert p.stat().st_size==expected, f'{p}: expected {expected}, got {p.stat().st_size}'
    assert p.stat().st_mode&0o111, f'{p}: not executable'
    print(f'{p}: {expected:,} bytes')
PY
