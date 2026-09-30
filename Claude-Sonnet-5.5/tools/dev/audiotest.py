#!/usr/bin/env python3
"""audiotest.py <demo> [seconds]: end-to-end audio check through the real PipeWire path.

Runs the finished executable with its audio routed to the virtual loopback sink (nothing is heard on the user's speakers), records that
sink's monitor with pw-record, aligns the recording with the reference render of the score (build/<demo>/aud.f32, made by gltool) and reports
the alignment delay, the level match and any dropouts. Needs numpy. The fullscreen window of the demo is shown as usual."""
import os, subprocess, sys, time, struct
import numpy as np
root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
demo = sys.argv[1]; secs = float(sys.argv[2]) if len(sys.argv) > 2 else 64
sink = 'alsa_output.platform-snd_aloop.0.analog-stereo'
out = os.path.join(root, 'build', 'audiotest_%s.wav' % demo)
rec = subprocess.Popen(['pw-record', '--target', sink, '-P', 'stream.capture.sink=true', '--rate', '48000', '--channels', '2', '--format', 'f32', out],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(1.5)
env = dict(os.environ, PIPEWIRE_NODE=sink)
t0 = time.monotonic()
p = subprocess.Popen([os.path.join(root, 'morphdemo-' + demo)], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
try: p.wait(timeout=secs)
except subprocess.TimeoutExpired: p.kill()
time.sleep(0.8); rec.terminate(); rec.wait()
raw = open(out, 'rb').read()
i = raw.find(b'data'); n = struct.unpack('<I', raw[i + 4:i + 8])[0]
a = np.frombuffer(raw[i + 8:i + 8 + (len(raw) - i - 8) // 8 * 8], dtype=np.float32).reshape(-1, 2)
ref = np.fromfile(os.path.join(root, 'build', demo, 'aud.f32'), dtype=np.float32).reshape(-1, 2)
print('recorded %.2f s (%d frames), demo wall time %.2f s, exit code %s' % (len(a) / 48000, len(a), time.monotonic() - t0, p.returncode))
# find the start: first sample above the noise floor of the recording
m = np.abs(a).max(axis=1)
# align by cross-correlation of the first 6 s of the mono mixes (FFT)
L = 6 * 48000
r = ref[:L].mean(axis=1).astype(np.float64)
best = None
seg = a[:L + 3 * 48000].mean(axis=1).astype(np.float64)
nfft = 1 << int(np.ceil(np.log2(len(seg) + len(r))))
X = np.fft.rfft(seg, nfft); Y = np.fft.rfft(r, nfft)
cc = np.fft.irfft(X * np.conj(Y), nfft)
lag = int(np.argmax(cc[:len(seg) - 1000]))
print('recording lags the reference by %d samples = %.1f ms (start of the queue playback in the recording; includes the wait before the demo started)' % (lag, lag / 48.0))
b = a[lag:lag + len(ref)]
n = min(len(b), len(ref)); b = b[:n]; rr = ref[:n]
err = b - rr
snr = 10 * np.log10((rr ** 2).sum() / max((err ** 2).sum(), 1e-12))
print('after alignment: %d frames compared, signal to error %.1f dB, max abs error %.4f, peak rec %.3f ref %.3f' % (n, snr, np.abs(err).max(), np.abs(b).max(), np.abs(rr).max()))
# dropouts: windows of 20 ms where the recording is silent while the reference is not
w = 960; bad = 0; where = []
for s in range(0, n - w, w):
    if np.abs(rr[s:s + w]).max() > 0.02 and np.abs(b[s:s + w]).max() < 1e-4: bad += 1; where.append(round(s / 48000, 2))
print('silent windows (20 ms) where the score is not silent: %d %s' % (bad, where[:10]))
# error per 5 s block
blk = [10 * np.log10((rr[s:s + 240000] ** 2).sum() / max((err[s:s + 240000] ** 2).sum(), 1e-12)) for s in range(0, n - 240000, 240000)]
print('signal to error per 5 s block (dB): ' + ' '.join('%.0f' % v for v in blk))
