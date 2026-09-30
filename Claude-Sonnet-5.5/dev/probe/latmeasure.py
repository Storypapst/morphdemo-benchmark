#!/usr/bin/env python3
"""Measure the latency between SDL_PauseAudioDevice and a click showing up on a PipeWire sink monitor."""
import os, subprocess, sys, time, select, struct
import numpy as np
sink = 'alsa_output.platform-snd_aloop.0.analog-stereo'
samples = sys.argv[1] if len(sys.argv) > 1 else '0'
root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
rec = subprocess.Popen(['pw-record', '--target', sink, '-P', 'stream.capture.sink=true', '--rate', '48000', '--channels', '2', '--format', 'f32', '--latency', '256/48000', '-'],
                       stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
time.sleep(1.0)
env = dict(os.environ, PIPEWIRE_NODE=sink)
p = subprocess.Popen([os.path.join(root, 'build', 'latprobe'), samples], stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env, text=True)
line = p.stdout.readline(); t_unpause = float(line.split()[0]); click_at = float(line.split()[1])
fd = rec.stdout.fileno(); data = b''; t0 = time.monotonic(); arrivals = []
os.set_blocking(fd, False)
while time.monotonic() - t0 < 3.0:
    r, _, _ = select.select([fd], [], [], 0.05)
    if r:
        try: chunk = os.read(fd, 65536)
        except BlockingIOError: continue
        if chunk: arrivals.append((time.monotonic(), len(data), len(chunk))); data += chunk
p.wait(); rec.terminate()
if len(data) < 1000: print('no data recorded'); sys.exit(1)
# skip a possible WAV/raw header: find float samples via alignment of 8 bytes
n = len(data) // 8 * 8
a = np.frombuffer(data[:n], dtype=np.float32).reshape(-1, 2)
env_ = np.abs(a).max(axis=1)
idx = np.where(env_ > 0.2)[0]
if not len(idx): print('no click found'); sys.exit(1)
first = idx[0]
byte_pos = first * 8
# time at which the chunk containing this sample was read
t_read = None
for tm, start, ln in arrivals:
    if start <= byte_pos < start + ln: t_read = tm + 0.0; t_end_chunk = start + ln; break
# capture side latency: the chunk was read at t_read, the click sits (t_end_chunk - byte_pos)/8 frames before the end of the chunk
t_click_capture = t_read - (t_end_chunk - byte_pos) / 8 / 48000.0
print('samples=%s: click arrives %.1f ms after unpause (expected content offset %.0f ms) => path latency ~ %.1f ms (incl. capture side)' % (samples, (t_click_capture - t_unpause) * 1e3, click_at * 1e3, (t_click_capture - t_unpause - click_at) * 1e3))
