#!/usr/bin/env python3
"""framestat.py frames.log: frame pacing statistics from the hook's log (real time, demo time per presented frame)."""
import sys, numpy as np
rows = []
for line in open(sys.argv[1]):
    p = line.split()
    if len(p) == 2:
        try: rows.append((float(p[0]), float(p[1])))
        except ValueError: pass
a = np.array(rows); r = a[:, 0]; d = a[:, 1]; dt = np.diff(r)
print('frames %d, real span %.2f s, demo span %.2f s' % (len(r), r[-1] - r[0], d[-1] - d[0]))
print('frame time ms: mean %.2f  p50 %.2f  p95 %.2f  p99 %.2f  max %.2f   fps %.1f' % (dt.mean() * 1e3, np.percentile(dt, 50) * 1e3, np.percentile(dt, 95) * 1e3, np.percentile(dt, 99) * 1e3, dt.max() * 1e3, 1 / dt.mean()))
for k in range(0, int(d[-1]) + 1, 10):
    m = (d >= k) & (d < k + 10)
    if m.sum() > 2:
        x = np.diff(r[m]); print('  demo %2d-%2d s: %.2f ms mean, %.2f ms max' % (k, k + 10, x.mean() * 1e3, x.max() * 1e3))
