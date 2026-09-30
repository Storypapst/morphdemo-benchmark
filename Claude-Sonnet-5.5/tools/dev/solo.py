#!/usr/bin/env python3
"""solo.py <demo> <layer,layer,...> <t0> <t1>: render single layers of a demo's dry bus (aud.comp variable names) and print their pitch-class profile and level.
The layer names are the local variables of aud.comp that are added to `dry`/`m` (for example pd, st, bl2, ld, ch)."""
import subprocess, sys, os, numpy as np
root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
demo, layers, t0, t1 = sys.argv[1], sys.argv[2].split(','), float(sys.argv[3]), float(sys.argv[4])
src = open(os.path.join(root, 'src', demo, 'aud.comp')).read()
tail = '  o[i]=vec4(dry,snd);'
if tail not in src: tail = '  o[i]=vec4(m,snd);'
names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
env = dict(os.environ, STRIDE16='1')
for L in layers:
    v = src.replace(tail, '  o[i]=vec4(%s,%s,0.,0.);' % ((L,) * 2 if L in ('kd', 'bass', 'sn2') else (L, '0.')) if False else '  o[i]=vec4(%s,0.,0.);' % L)
    open(os.path.join(root, 'build', demo, 'solo.comp'), 'w').write(v)
    with open(os.path.join(root, 'build', demo, 'solo.min'), 'w') as f:
        subprocess.run(['python3', os.path.join(root, 'tools/glslmin.py'), '-p', os.path.join(root, 'src', demo, 'common.glsl'), os.path.join(root, 'build', demo, 'solo.comp')], stdout=f, check=True)
    r = subprocess.run([os.path.join(root, 'build/gltool'), 'audio', os.path.join(root, 'build', demo, 'solo.min'), '60', '48000', os.path.join(root, 'build', demo, 'solo.f32')], capture_output=True, text=True, env=env)
    if 'FAILED' in r.stderr or 'error' in r.stderr.lower(): print(L, 'compile issue', r.stderr[:300]); continue
    a = np.fromfile(os.path.join(root, 'build', demo, 'solo.f32'), dtype=np.float32).reshape(-1, 4)[:, :2].mean(axis=1)
    x = a[int(t0 * 48000):int(t1 * 48000)]
    n = 1 << int(np.ceil(np.log2(len(x)))); sp = np.abs(np.fft.rfft(x * np.hanning(len(x)), n)) ** 2; fr = np.fft.rfftfreq(n, 1 / 48000)
    m = (fr > 60) & (fr < 4000); ch = np.zeros(12)
    for p, v in zip(np.round(69 + 12 * np.log2(fr[m] / 440.0)).astype(int) % 12, sp[m]): ch[p] += v
    ch /= ch.sum() + 1e-30; top = np.argsort(ch)[::-1][:5]
    print('%-6s rms %6.1f dBFS  top pitch classes: %s' % (L, 20 * np.log10(np.sqrt((x ** 2).mean()) + 1e-9), '  '.join('%s %.0f%%' % (names[i], 100 * ch[i]) for i in top)))
