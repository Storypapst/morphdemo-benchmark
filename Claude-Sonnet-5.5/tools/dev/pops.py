#!/usr/bin/env python3
"""pops.py <dir>: frame-to-frame change of a shot series (shot_*.ppm): mean absolute difference, mean luminance step and a list of the largest jumps.
Used to find unintended cuts and pops in a filmstrip taken with HOOK_EVERY."""
import sys, glob, os, re
import numpy as np
from PIL import Image
d = sys.argv[1]; top = int(sys.argv[2]) if len(sys.argv) > 2 else 25
files = []
for f in glob.glob(os.path.join(d, 'shot_*.ppm')):
    m = re.search(r'shot_([\d.]+)\.ppm', f)
    if m: files.append((float(m.group(1)), f))
files.sort()
prev = None; rows = []
for t, f in files:
    a = np.asarray(Image.open(f).convert('RGB'), dtype=np.float32) / 255
    lum = float((0.2126 * a[..., 0] + 0.7152 * a[..., 1] + 0.0722 * a[..., 2]).mean())
    if prev is not None:
        # compare after a 4x4 box blur so that grain and static do not dominate
        h, w, _ = a.shape; k = 4
        A = a[:h // k * k, :w // k * k].reshape(h // k, k, w // k, k, 3).mean(axis=(1, 3)); B = prev[0][:h // k * k, :w // k * k].reshape(h // k, k, w // k, k, 3).mean(axis=(1, 3))
        rows.append((t, float(np.abs(A - B).mean()), lum - prev[1]))
    prev = (a, lum)
r = np.array(rows)
print('%d frames, mean change %.4f, median %.4f, 95th percentile %.4f' % (len(r), r[:, 1].mean(), np.median(r[:, 1]), np.percentile(r[:, 1], 95)))
order = np.argsort(r[:, 1])[::-1][:top]
print('largest frame-to-frame changes (time, mean abs diff, luminance step):')
for i in sorted(order): print('  t=%6.2f  diff %.4f  dlum %+.3f' % tuple(r[i]))
