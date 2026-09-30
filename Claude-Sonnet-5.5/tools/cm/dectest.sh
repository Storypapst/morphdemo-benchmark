#!/usr/bin/env bash
# dectest.sh <input> <params.txt>: compress with the reference encoder, decode with the asm decoder in a test ELF, compare
set -euo pipefail
here="$(cd "$(dirname "$0")/../.." && pwd)"; in=$1; par=$2; t="$here/build/cm/t"; mkdir -p "$t"
"$here/build/cmtool" enc "$in" "$par" "$t/s.cm"
python3 "$here/tools/cm/mkparams.py" "$par" > "$t/params.inc"
n=$(stat -c %s "$in")
sed "s/INLEN_PLACEHOLDER/$n/; s#STREAMFILE#$t/s.cm#" "$here/tools/cm/dec4test.asm" > "$t/dec4test.asm"
(cd "$t" && nix-shell "$here/shell.nix" --run "nasm -f bin -I$t/ -I$here/tools/cm/ -o dec4test dec4test.asm" 2>&1 | grep -v deprecated || true)
chmod +x "$t/dec4test"; "$t/dec4test" > "$t/out.bin"
cmp "$in" "$t/out.bin" && echo "asm decoder OK ($(stat -c %s "$t/s.cm") bytes -> $n)"
