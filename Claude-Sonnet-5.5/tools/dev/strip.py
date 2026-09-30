#!/usr/bin/env python3
"""strip.py <dir> <out_prefix> <t0> <t1> [cols rows]: contact sheets from shot_*.ppm files, one sheet per cols*rows frames in [t0,t1]. Also prints mean luminance stats."""
import sys, glob, os, re
import numpy as np
from PIL import Image, ImageDraw
d, pre, t0, t1 = sys.argv[1], sys.argv[2], float(sys.argv[3]), float(sys.argv[4])
cols = int(sys.argv[5]) if len(sys.argv) > 5 else 6; rows = int(sys.argv[6]) if len(sys.argv) > 6 else 4
files = []
for f in glob.glob(os.path.join(d, 'shot_*.ppm')):
    m = re.search(r'shot_([\d.]+)\.ppm', f)
    if m and t0 <= float(m.group(1)) <= t1: files.append((float(m.group(1)), f))
files.sort()
per = cols * rows
for si in range(0, len(files), per):
    chunk = files[si:si + per]
    ims = [Image.open(f).convert('RGB') for _, f in chunk]
    w, h = ims[0].size
    sheet = Image.new('RGB', (cols * w, rows * h), (20, 20, 20)); dr = ImageDraw.Draw(sheet)
    for i, ((t, _), im) in enumerate(zip(chunk, ims)):
        x, y = (i % cols) * w, (i // cols) * h; sheet.paste(im, (x, y)); dr.rectangle([x, y, x + 44, y + 12], fill=(0, 0, 0)); dr.text((x + 2, y), '%.1f' % t, fill=(255, 255, 0))
    out = '%s_%02d.png' % (pre, si // per); sheet.save(out); print(out, sheet.size, 'frames %.1f..%.1f' % (chunk[0][0], chunk[-1][0]))
lum = []
for t, f in files:
    a = np.asarray(Image.open(f).convert('RGB'), dtype=np.float32) / 255
    lum.append((t, float((0.2126 * a[..., 0] + 0.7152 * a[..., 1] + 0.0722 * a[..., 2]).mean())))
if lum:
    L = np.array([v for _, v in lum]); dL = np.abs(np.diff(L))
    print('mean luminance range %.3f..%.3f, largest step between consecutive shots %.3f at t=%.2f' % (L.min(), L.max(), dL.max() if len(dL) else 0, lum[int(dL.argmax()) + 1][0] if len(dL) else 0))
