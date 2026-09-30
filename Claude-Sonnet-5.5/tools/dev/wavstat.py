#!/usr/bin/env python3
"""wavstat.py <in.f32> [rate] [out_prefix]: level per second, spectrogram PNG, 16-bit WAV."""
import sys, numpy as np, wave
from PIL import Image
fn = sys.argv[1]; rate = int(sys.argv[2]) if len(sys.argv) > 2 else 48000
pre = sys.argv[3] if len(sys.argv) > 3 else fn.rsplit('.', 1)[0]
a = np.fromfile(fn, dtype=np.float32).reshape(-1, 2)
n = len(a); print('frames', n, 'seconds', n / rate, 'peak', np.abs(a).max(), 'dc', a.mean(axis=0))
print('nan/inf:', np.isnan(a).sum(), np.isinf(a).sum())
mono = a.mean(axis=1)
for s in range(0, int(n / rate), 2):
    x = mono[s * rate:(s + 2) * rate]
    if len(x) == 0: break
    rms = np.sqrt((x ** 2).mean()); pk = np.abs(a[s * rate:(s + 2) * rate]).max()
    spec = np.abs(np.fft.rfft(x * np.hanning(len(x)))); fr = np.fft.rfftfreq(len(x), 1 / rate)
    cen = (spec * fr).sum() / max(spec.sum(), 1e-9)
    pw = spec ** 2; tot = max(pw.sum(), 1e-12)
    bands = [(0, 120), (120, 300), (300, 1000), (1000, 4000), (4000, 24000)]
    bp = ' '.join('%4.1f' % (10 * np.log10(max(pw[(fr >= a) & (fr < b)].sum() / tot, 1e-6))) for a, b in bands)
    print('%3d-%3ds rms %6.1f dBFS peak %6.1f  centroid %6.0f Hz  band power dB (sub,bass,lowmid,mid,high): %s' % (s, s + 2, 20 * np.log10(rms + 1e-9), 20 * np.log10(pk + 1e-9), cen, bp))
# spectrogram (log-frequency, interpolated, colour mapped)
nfft = 4096; hop = 2048
win = np.hanning(nfft)
frames = []
for i in range(0, n - nfft, hop):
    frames.append(np.abs(np.fft.rfft(mono[i:i + nfft] * win)))
S = np.array(frames).T
fr = np.fft.rfftfreq(nfft, 1 / rate)
fa = np.geomspace(40, 14000, 320)
img = np.array([np.interp(fa, fr, S[:, c]) for c in range(S.shape[1])]).T
db = 20 * np.log10(img + 1e-7)
db = np.clip((db - (db.max() - 75)) / 75, 0, 1)
# inferno-like palette
stops = np.array([[0,0,4],[40,11,84],[101,21,110],[159,42,99],[212,72,66],[245,125,21],[250,193,39],[252,255,164]], dtype=float)
xs = np.linspace(0, 1, len(stops))
rgb = np.stack([np.interp(db, xs, stops[:, k]) for k in range(3)], axis=-1).astype(np.uint8)
im = Image.fromarray(rgb[::-1])
im = im.resize((min(1500, db.shape[1]), 320), Image.BILINEAR)
# time ticks every 10 s
from PIL import ImageDraw
d = ImageDraw.Draw(im); W = im.size[0]; dur = n / rate
for sec in range(0, int(dur) + 1, 10):
    x = int(sec / dur * (W - 1)); d.line([x, 0, x, 8], fill=(255, 255, 255)); d.text((x + 2, 8), str(sec) + 's', fill=(255, 255, 255))
for f in (100, 1000, 10000):
    y = int(319 - (np.log(f / 40) / np.log(14000 / 40)) * 319); d.line([0, y, 6, y], fill=(255, 255, 255)); d.text((8, y - 5), str(f), fill=(255, 255, 255))
im.save(pre + '_spec.png'); print('spectrogram', pre + '_spec.png')
w = wave.open(pre + '.wav', 'wb'); w.setnchannels(2); w.setsampwidth(2); w.setframerate(rate)
w.writeframes((np.clip(a, -1, 1) * 32767).astype('<i2').tobytes()); w.close(); print('wav', pre + '.wav')
