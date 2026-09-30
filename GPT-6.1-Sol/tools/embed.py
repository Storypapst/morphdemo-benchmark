#!/usr/bin/env python3
"""Embed human-readable GLSL into C, removing only comments and whitespace."""
import json
from pathlib import Path
import re
import sys

target = Path(sys.argv[1])
source = '\n'.join(Path(p).read_text() for p in sys.argv[2:])
source = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
# Keep preprocessor lines separate. No shader minifier is needed to rebuild.
lines = []
for line in source.splitlines():
    line = line.strip()
    if not line:
        continue
    if line.startswith('#'):
        lines.append('\n' + line + '\n')
    else:
        line = re.sub(r'\s+', ' ', line)
        line = re.sub(r'\s*([{}();,+*/=<>?:&|\[\]])\s*', r'\1', line)
        lines.append(line)
target.parent.mkdir(parents=True, exist_ok=True)
target.write_text('static const char fragment_shader[] = ' + json.dumps(''.join(lines)) + ';\n')
