#!/usr/bin/env python3
"""mkparams.py <params.txt> [outlen]: emit the NASM constants and parameter table for dec4.inc."""
import sys
d = {}
for line in open(sys.argv[1]):
    k, *v = line.split()
    d[k] = [int(x) for x in v]
nm = d['nm'][0]
print('%%define NM %d' % nm)
print('%%define A0 %d' % d['a0'][0])
print('%%define CAP %d' % d['cap'][0])
print('%%define NS %d' % d['ns'][0])
print('%%define TMASK 0x%x' % ((1 << d['tbits'][0]) - 1))
print('%%define ACC0 %d' % (d['a0'][0] * 65537))
print('%macro PARAMS 0')
print('  db ' + ','.join(str(x) for x in d['mask']))
print('  db ' + ','.join(str(x) for x in d['wt']))
print('%endmacro')
