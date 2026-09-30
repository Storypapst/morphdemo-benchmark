#!/usr/bin/env python3
"""mkshader.py <common.glsl> <body> <out>: prepend the shared prelude after the #version line of body."""
import sys
common, body, out = sys.argv[1:4]
b = open(body).read().split('\n')
ver = b[0]
assert ver.startswith('#version')
rest = '\n'.join(b[1:])
# keep layout/uniform declarations of the body first (they precede the prelude use), the prelude only defines functions
open(out, 'w').write(ver + '\n' + open(common).read() + rest)
