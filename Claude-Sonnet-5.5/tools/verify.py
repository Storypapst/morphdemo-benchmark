#!/usr/bin/env python3
"""verify.py: acceptance checks for the three executables.

    verify.py sizes            exact byte sizes and ELF sanity (static checks, instant)
    verify.py run [names...]   run every production from start to end in real time (about a minute each), report wall time and exit code
    verify.py escape [names]   start each production, inject an Escape key press after 4 s through the LD_PRELOAD hook, expect a clean quick exit
    verify.py content [names]  run each production with a 10x faster clock under the hook: the payload unpacked inside the executable must equal the
                               build output byte for byte, every shader must link, the queued audio must be exactly
                               60 s of finite, non-silent stereo float, frames must be presented, and the process must exit by itself with code 0
    verify.py all              all of the above (about four minutes)

The hook (build/hook.so, made by `make tools`) is used for the Escape test only; the `run` check runs the plain executables.
"""
import os, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SIZES = {'morphdemo-64k': 65536, 'morphdemo-16k': 16384, 'morphdemo-4k': 4096}

def sizes():
    ok = True
    for n, want in SIZES.items():
        p = os.path.join(ROOT, n)
        if not os.path.exists(p):
            print('%-14s MISSING' % n); ok = False; continue
        got = os.path.getsize(p)
        magic = open(p, 'rb').read(4)
        exe = os.access(p, os.X_OK)
        good = got == want and magic == b'\x7fELF' and exe
        ok &= good
        print('%-14s %6d bytes (want %6d)  ELF=%s  executable=%s  %s' % (n, got, want, magic == b'\x7fELF', exe, 'OK' if good else 'FAIL'))
    return ok

def run(names):
    ok = True
    for n in names:
        p = os.path.join(ROOT, n)
        t0 = time.monotonic()
        r = subprocess.run([p], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        dt = time.monotonic() - t0
        good = r.returncode == 0 and 50.0 <= dt <= 70.0
        ok &= good
        print('%-14s exit %d, wall time %.2f s  %s %s' % (n, r.returncode, dt, 'OK' if good else 'FAIL', r.stderr.decode()[:200].strip()))
    return ok

def escape(names):
    hook = os.path.join(ROOT, 'build', 'hook.so')
    if not os.path.exists(hook):
        print('build/hook.so missing: run `make tools`'); return False
    ok = True
    for n in names:
        p = os.path.join(ROOT, n)
        env = dict(os.environ, LD_PRELOAD=hook, HOOK_ESC='4')
        t0 = time.monotonic()
        r = subprocess.run([p], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        dt = time.monotonic() - t0
        good = r.returncode == 0 and 3.5 <= dt <= 8.0 and b'injected Escape' in r.stderr
        ok &= good
        print('%-14s Escape injected at 4 s: exit %d after %.2f s  %s' % (n, r.returncode, dt, 'OK' if good else 'FAIL'))
    return ok

def content(names):
    import re, tempfile
    import numpy as np
    hook = os.path.join(ROOT, 'build', 'hook.so')
    if not os.path.exists(hook):
        print('build/hook.so missing: run `make tools`'); return False
    ok = True
    for n in names:
        p = os.path.join(ROOT, n)
        with tempfile.TemporaryDirectory() as d:
            payload = os.path.join(ROOT, 'build', n.split('-')[1], 'payload.bin')
            dest = '0x401000' if n.endswith('-4k') else '0x410000'
            dump = os.path.join(d, 'unpacked.bin')
            env = dict(os.environ, LD_PRELOAD=hook, HOOK_SPEED='10', HOOK_AUDIO='1', HOOK_LOG='1', HOOK_OUT=d, HOOK_WINDOW='1920x1080',
                       HOOK_DUMP='%s:%d:%s' % (dest, os.path.getsize(payload), dump) if os.path.exists(payload) else '')
            t0 = time.monotonic()
            r = subprocess.run([p], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
            dt = time.monotonic() - t0
            err = r.stderr
            problems = []; a_desc = '-'
            if r.returncode != 0: problems.append('exit code %d' % r.returncode)
            if re.search(r'link=0|FAILED', err): problems.append('a shader failed to build: ' + ' | '.join(l for l in err.splitlines() if 'link=0' in l or 'FAILED' in l or 'error' in l)[:300])
            if not re.search(r'link=1', err) and 'glLinkProgram' not in err: problems.append('no shader program reported')
            if os.path.exists(payload):
                if not os.path.exists(dump): problems.append('the unpacked payload could not be dumped')
                elif open(dump, 'rb').read() != open(payload, 'rb').read(): problems.append('the payload unpacked inside the executable differs from build/%s/payload.bin' % n.split('-')[1])
            af = os.path.join(d, 'audio.raw')
            if not os.path.exists(af): problems.append('no audio was queued')
            else:
                a = np.fromfile(af, dtype=np.float32)
                frames = len(a) // 2
                if frames != 2880000: problems.append('audio has %d frames (want 2880000 = 60 s at 48 kHz)' % frames)
                a = a[:frames * 2].reshape(-1, 2)
                if not np.isfinite(a).all(): problems.append('audio contains NaN or Inf')
                if np.abs(a).max() > 1.0: problems.append('audio peak %.3f exceeds full scale' % np.abs(a).max())
                blocks = [20 * np.log10(np.sqrt((a[i * 480000:(i + 1) * 480000] ** 2).mean()) + 1e-9) for i in range(6)]
                if min(blocks) < -32: problems.append('a 10 s block of the audio is nearly silent: %s dBFS' % ['%.1f' % b for b in blocks])
                a_desc = 'audio 60 s, block RMS dBFS ' + ' '.join('%.0f' % b for b in blocks)
            fl = os.path.join(d, 'frames.log')
            nfr = sum(1 for _ in open(fl)) if os.path.exists(fl) else 0
            if nfr < 120: problems.append('only %d frames presented' % nfr)
        good = not problems
        ok &= good
        print('%-14s %s  (%s, %d frames, %.1f s at 10x)  %s' % (n, 'OK' if good else 'FAIL', a_desc, nfr, dt, '; '.join(problems)))
    return ok

if __name__ == '__main__':
    cmd = sys.argv[1] if len(sys.argv) > 1 else 'sizes'
    names = sys.argv[2:] or list(SIZES)
    res = True
    if cmd in ('sizes', 'all'): res &= sizes()
    if cmd in ('run', 'all'): res &= run(names)
    if cmd in ('escape', 'all'): res &= escape(names)
    if cmd in ('content', 'all'): res &= content(names)
    sys.exit(0 if res else 1)
