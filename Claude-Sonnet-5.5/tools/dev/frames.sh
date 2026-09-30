#!/usr/bin/env bash
# frames.sh <frag.glsl> <outdir> <W> <H> <cols> t1 t2 ...   -> outdir/sheet.png
set -euo pipefail
here="$(cd "$(dirname "$0")/../.." && pwd)"
frag=$1; out=$2; W=$3; H=$4; cols=$5; shift 5
mkdir -p "$out"; rm -f "$out"/f_*.ppm
"$here/build/gltool" render "$frag" "$out/f" "$W" "$H" "$@"
labels=$(printf '%s,' "$@"); labels=${labels%,}
LABELS="$labels" python3 "$here/tools/dev/sheet.py" "$out/sheet.png" "$cols" "$out"/f_*.ppm
