#!/usr/bin/env python3
"""pack.py: build a tiny self-unpacking ELF.

    pack.py --payload payload.bin --params cm.params --libs libSDL2-2.0.so.0,libGL.so.1 --size 4096 --out morphdemo-4k

Compresses the payload with the cm4 model (tools/cm) and wraps it in the hand-made ELF stub (tools/stub/stub.asm).
The compressed stream sits at the very end of the file (the decoder reads zeros past the end), padding goes before it.
"""
import argparse, os, re, subprocess, sys

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def nasm(args):
    r = subprocess.run([os.path.join(root, 'tools', 'nasm.sh')] + args, capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit('nasm failed:\n' + r.stderr)
    return r

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--payload', required=True)
    ap.add_argument('--params', required=True)
    ap.add_argument('--libs', required=True)
    ap.add_argument('--size', type=int, required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--work', default=None)
    ap.add_argument('--dest', default='0x401000')
    ap.add_argument('--table', default='0x1000000')
    ap.add_argument('--memsize', default='0')
    a = ap.parse_args()
    work = a.work or os.path.join(root, 'build', 'pack')
    os.makedirs(work, exist_ok=True)
    cmtool = os.path.join(root, 'build', 'cmtool')
    plen = os.path.getsize(a.payload)
    if int(a.dest, 0) + plen > 0x440000:
        sys.exit('payload of %d bytes at %s would run into the import table at 0x440000' % (plen, a.dest))
    tbits = int([l.split()[1] for l in open(a.params) if l.startswith('tbits')][0])
    need = int(a.table, 0) + (4 << tbits) - 0x400000            # the model slots live in the zero-filled tail of the segment
    memsize = max(int(a.memsize, 0), need)
    stream = os.path.join(work, 'stream.cm')
    r = subprocess.run([cmtool, 'enc', a.payload, a.params, stream], capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit('cmtool failed: ' + r.stdout + r.stderr)
    slen = os.path.getsize(stream)
    inc = subprocess.run([sys.executable, os.path.join(root, 'tools/cm/mkparams.py'), a.params], capture_output=True, text=True, check=True).stdout
    libs = a.libs.split(',')
    inc += '%macro LIBS_ENTRIES 0\n' + ''.join('  dq 1, lib%d - strtab\n' % i for i in range(len(libs))) + '%endmacro\n'
    inc += '%macro LIBS_NAMES 0\n' + ''.join('  lib%d: db "%s",0\n' % (i, l) for i, l in enumerate(libs)) + '%endmacro\n%define LIBS 1\n'
    open(os.path.join(work, 'params.inc'), 'w').write(inc)
    def build(streampos, out, lst=None):
        args = ['-f', 'bin', '-I', work + '/', '-I', os.path.join(root, 'tools/cm') + '/', '-DPAYLEN=%d' % plen, '-DDEST=%s' % a.dest, '-DTABLE=%s' % a.table, '-DMEMSIZE=%d' % memsize, '-DSTREAMFILE="%s"' % stream, '-DSTREAMPOS=%d' % streampos, '-o', out]
        if lst: args += ['-l', lst]
        nasm(args + [os.path.join(root, 'tools/stub/stub.asm')])
    lst = os.path.join(work, 'stub.lst')
    build(0x1000, os.path.join(work, 'probe.bin'), lst)
    codeend = None
    for line in open(lst):
        m = re.match(r'\s*\d+\s+([0-9A-F]{8})\s+[0-9A-F()]+\s+call\s+DEST', line)
        if m: codeend = int(m.group(1), 16) + 5
    assert codeend, 'cannot find the end of the decoder in the listing'
    streampos = a.size - slen
    if streampos < codeend:
        sys.exit('too big: stub+decoder %d + stream %d = %d > %d (over by %d)' % (codeend, slen, codeend + slen, a.size, codeend + slen - a.size))
    build(streampos, a.out)
    exe = open(a.out, 'rb').read()
    assert len(exe) == a.size, (len(exe), a.size)
    os.chmod(a.out, 0o755)
    print('%s: %d bytes (stub+decoder %d, padding %d, stream %d; payload %d -> %.1f%%)' % (os.path.basename(a.out), len(exe), codeend, streampos - codeend, slen, plen, 100.0 * slen / plen))

main()
