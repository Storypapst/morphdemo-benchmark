#!/usr/bin/env python3
"""shots.py <out.png> <cols> <maxw> <maxh> file.ppm ...: downscaled contact sheet of captured frames (fit into maxw x maxh cells)."""
import sys, os
from PIL import Image, ImageDraw
out, cols, mw, mh = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]); files = sys.argv[5:]
ims = []
for f in files:
    im = Image.open(f).convert('RGB'); s = min(mw / im.size[0], mh / im.size[1]); im = im.resize((max(1, int(im.size[0] * s)), max(1, int(im.size[1] * s))), Image.LANCZOS); ims.append((os.path.basename(f), im))
rows = (len(ims) + cols - 1) // cols
sheet = Image.new('RGB', (cols * mw, rows * mh), (30, 30, 30)); d = ImageDraw.Draw(sheet)
for i, (name, im) in enumerate(ims):
    x, y = (i % cols) * mw, (i // cols) * mh; sheet.paste(im, (x, y)); d.text((x + 3, y + 2), name, fill=(255, 255, 0))
sheet.save(out); print(out, sheet.size)
