#!/usr/bin/env bash
# audcheck.sh <4k|16k|...> [stride]: minify the audio shader, render it on the GPU, print the analysis table (every <stride> rows)
set -euo pipefail
here="$(cd "$(dirname "$0")/../.." && pwd)"; d=$1; stride=${2:-2}
src="$here/src/$d/aud.comp"; out="$here/build/$d"; mkdir -p "$out"
python3 "$here/tools/glslmin.py" -p "$here/src/$d/common.glsl" "$src" > "$out/aud_dev.min"   # never write aud.min: that name belongs to the Makefile
"$here/build/gltool" audio "$out/aud_dev.min" 60 48000 "$out/aud.f32" 2>&1 | grep -E "FAILED|error|C[0-9]{4}" || true
python3 "$here/tools/dev/wavstat.py" "$out/aud.f32" 48000 | awk -v s="$stride" 'NR<=2 || (NR>2 && (NR-3)%s==0) || /spectrogram|wav/'
