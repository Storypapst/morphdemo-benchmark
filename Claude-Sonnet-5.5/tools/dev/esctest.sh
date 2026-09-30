#!/usr/bin/env bash
# esctest.sh <demo executable> [driver]: real Escape key test on the live KDE session.
# Starts the demo, activates its window with kdotool, checks that it really is the active window, and only then injects a genuine
# Escape key press through a private ydotoold (uinput). If the demo window cannot be activated nothing is typed.
# NOACTIVATE=1 skips the kdotool activation: the test then shows whether the window gets keyboard focus by itself (it does when the session is idle;
# KWin focus stealing prevention denies it while the user is typing in another window).
set -uo pipefail
exe=$1; drv=${2:-}
YD=$(ls -d /nix/store/*-ydotool-1.0.4/bin | head -1); KD=$(ls -d /nix/store/*-kdotool-0.2.3/bin | head -1)/kdotool
sock=/run/user/$(id -u)/morphdemo-esctest-$$.sock
$YD/ydotoold -p "$sock" >/dev/null 2>&1 & yd=$!
trap 'kill $yd 2>/dev/null; rm -f "$sock"; [ -n "${pid:-}" ] && kill $pid 2>/dev/null' EXIT
for i in 1 2 3 4 5 6 7 8 9 10; do [ -S "$sock" ] && break; sleep 0.2; done
[ -S "$sock" ] || { echo "ydotoold did not start"; exit 2; }
if [ -n "$drv" ]; then SDL_VIDEODRIVER=$drv "$exe" >/dev/null 2>&1 & else "$exe" >/dev/null 2>&1 & fi
pid=$!
sleep 2.5
kill -0 $pid 2>/dev/null || { echo "demo already gone"; exit 3; }
ids=$($KD search --all --pid $pid 2>/dev/null)
echo "windows of pid $pid: $(echo $ids | tr '\n' ' ')"
id=$(echo "$ids" | head -1)
if [ -z "${NOACTIVATE:-}" ]; then $KD windowactivate "$id" 2>&1 | head -2; fi
sleep 0.7
act=$($KD getactivewindow 2>/dev/null); actpid=$($KD getactivewindow getwindowpid 2>/dev/null)
echo "active window after activation: $act (pid $actpid), demo pid $pid"
if [ "$actpid" != "$pid" ]; then echo "demo window is not active: not typing anything"; exit 4; fi
t0=$(date +%s.%N)
YDOTOOL_SOCKET="$sock" $YD/ydotool key 1:1 1:0 >/dev/null 2>&1
for i in $(seq 1 30); do kill -0 $pid 2>/dev/null || break; sleep 0.1; done
if kill -0 $pid 2>/dev/null; then echo "RESULT: demo still running 3 s after Escape"; exit 5; fi
wait $pid; rc=$?
echo "RESULT: demo exited after Escape, exit code $rc, within $(python3 -c "import time;print('%.1f'%(time.time()-$t0))") s"
