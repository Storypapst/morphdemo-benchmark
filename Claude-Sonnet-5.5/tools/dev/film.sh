#!/usr/bin/env bash
# film.sh <4k|16k|64k> <tag> [every] [div] [w h]: run the finished executable at 8x clock speed in a 1920x1080 window and save a filmstrip
# into build/film/<demo>_<tag>/ (fresh directory per tag; nothing is deleted).
# SPEEDS="0:8,44:1,49:8" sets a piecewise clock speed by demo time (here: real time between 44 s and 49 s).
set -euo pipefail
here="$(cd "$(dirname "$0")/../.." && pwd)"
demo=$1; tag=$2; every=${3:-0.5}; div=${4:-4}; w=${5:-1920}; h=${6:-1080}
out="$here/build/film/${demo}_${tag}"; mkdir -p "$out"
cd "$out"
env HOOK_WINDOW="${w}x${h}" HOOK_SPEED=8 ${SPEEDS:+HOOK_SPEEDS="$SPEEDS"} HOOK_DIV="$div" HOOK_EVERY="$every" HOOK_OUT=. LD_PRELOAD="$here/build/hook.so" "$here/morphdemo-$demo" 2>&1 | grep -E "FAILED|error" || true
echo "$out: $(ls "$out" | wc -l) frames"
