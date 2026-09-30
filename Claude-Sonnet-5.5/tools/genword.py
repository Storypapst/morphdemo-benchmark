#!/usr/bin/env python3
"""genword.py: emit the GLSL table of line segments that form the word MORPH (each letter is 4 wide, 6 tall, pitch 6)."""
import math, sys
def arc(cx, cy, rx, ry, a0, a1, n):
    pts = [(cx + rx * math.cos(math.radians(a0 + (a1 - a0) * i / n)), cy + ry * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]
    return [(pts[i], pts[i + 1]) for i in range(n)]
L = {
 'M': [((0,0),(0,6)), ((0,6),(2,2.6)), ((2,2.6),(4,6)), ((4,6),(4,0))],
 'O': arc(2, 3, 2, 3, 0, 360, 20),
 'R': [((0,0),(0,6)), ((0,6),(2,6)), ((2,3),(0,3)), ((1.6,3),(4,0))] + arc(2, 4.5, 1.5, 1.5, 90, -90, 6),
 'P': [((0,0),(0,6)), ((0,6),(2,6)), ((2,3),(0,3))] + arc(2, 4.5, 1.5, 1.5, 90, -90, 6),
 'H': [((0,0),(0,6)), ((4,0),(4,6)), ((0,3),(4,3))],
}
segs = []
for i, ch in enumerate('MORPH'):
    ox = i * 6 - 14
    for (a, b) in L[ch]:
        segs.append((ox + a[0], a[1] - 3, ox + b[0], b[1] - 3))
def f(v):
    s = ('%.2f' % v).rstrip('0').rstrip('.')
    return s if s not in ('', '-0') else '0'
name = sys.argv[1] if len(sys.argv) > 1 else 'W'
print('const vec4 %s[%d]=vec4[](%s);' % (name, len(segs), ','.join('vec4(%s)' % ','.join(f(x) for x in s) for s in segs)))
