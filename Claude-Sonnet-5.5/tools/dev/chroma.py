#!/usr/bin/env python3
"""chroma.py <in.f32> <bar_seconds> [first_bar last_bar]: strongest pitch classes per bar (checks the harmony), and the kick period."""
import sys, numpy as np
fn = sys.argv[1]; bar = float(sys.argv[2]); rate = 48000
a = np.fromfile(fn, dtype=np.float32).reshape(-1, 2).mean(axis=1)
names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
b0 = int(sys.argv[3]) if len(sys.argv) > 3 else 0; b1 = int(sys.argv[4]) if len(sys.argv) > 4 else int(len(a) / rate / bar) - 1
for b in range(b0, b1 + 1):
    x = a[int(b * bar * rate):int((b + 1) * bar * rate)]
    if len(x) < 4096: break
    n = 1 << int(np.ceil(np.log2(len(x)))); sp = np.abs(np.fft.rfft(x * np.hanning(len(x)), n)); fr = np.fft.rfftfreq(n, 1 / rate)
    ch = np.zeros(12)
    m = (fr > 60) & (fr < 2000)
    midi = 69 + 12 * np.log2(fr[m] / 440.0); pc = np.round(midi).astype(int) % 12
    for p, v in zip(pc, sp[m] ** 2): ch[p] += v
    ch /= ch.sum() + 1e-12
    top = np.argsort(ch)[::-1][:4]
    print('bar %2d (%5.1fs): ' % (b, b * bar) + '  '.join('%s %.0f%%' % (names[i], 100 * ch[i]) for i in top))
# kick period from the low-band envelope autocorrelation (steady part)
lo = a[int(20 * rate):int(40 * rate)]
k = np.ones(200) / 200; env = np.convolve(np.abs(lo), k, 'same')
from numpy.fft import rfft, irfft
e = env - env.mean(); ac = irfft(np.abs(rfft(e, 2 * len(e))) ** 2)[:rate]
lag = np.argmax(ac[int(0.3 * rate):int(0.8 * rate)]) + int(0.3 * rate)
print('dominant rhythmic period %.4f s -> %.1f BPM' % (lag / rate, 60 / (lag / rate)))
