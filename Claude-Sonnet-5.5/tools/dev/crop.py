#!/usr/bin/env python3
"""crop.py in.ppm out.png [w h] [scale]: centre crop (default 1280x720) of a captured frame, optionally downscaled."""
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert('RGB'); W, H = im.size
w = int(sys.argv[3]) if len(sys.argv) > 3 else 1280; h = int(sys.argv[4]) if len(sys.argv) > 4 else 720
sc = float(sys.argv[5]) if len(sys.argv) > 5 else 1.0
x0 = max(0, (W - w) // 2); y0 = max(0, (H - h) // 2)
c = im.crop((x0, y0, min(W, x0 + w), min(H, y0 + h)))
if sc != 1.0: c = c.resize((int(c.size[0] * sc), int(c.size[1] * sc)), Image.LANCZOS)
c.save(sys.argv[2]); print(sys.argv[2], c.size, 'from', im.size)
