#!/usr/bin/env python3
"""Contact sheet from PPM frames: sheet.py out.png cols frame1.ppm frame2.ppm ... (labels optional via LABELS env, comma separated)"""
import sys, os
from PIL import Image, ImageDraw
out, cols = sys.argv[1], int(sys.argv[2]); files = sys.argv[3:]
labels = os.environ.get('LABELS', '').split(',') if os.environ.get('LABELS') else [os.path.basename(f) for f in files]
ims = [Image.open(f).convert('RGB') for f in files]
w, h = ims[0].size
rows = (len(ims) + cols - 1) // cols
sheet = Image.new('RGB', (cols * w, rows * h))
d = ImageDraw.Draw(sheet)
for i, im in enumerate(ims):
    x, y = (i % cols) * w, (i // cols) * h
    sheet.paste(im, (x, y))
    d.rectangle([x, y, x + 8 * len(labels[i]) + 6, y + 14], fill=(0, 0, 0))
    d.text((x + 3, y + 1), labels[i], fill=(255, 255, 0))
sheet.save(out)
print(out, sheet.size)
