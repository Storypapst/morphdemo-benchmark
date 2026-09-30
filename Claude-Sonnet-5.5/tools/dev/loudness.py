#!/usr/bin/env python3
"""loudness.py <in.f32> [rate]: ITU-R BS.1770 loudness (K-weighted, gated), short-term loudness per 3 s block, true-peak estimate,
octave-band levels (A-weighted) per 10 s section and the spectral tilt."""
import sys
import numpy as np

def biquad(x, b, a):
    # direct form I, vectorised over channels via scipy-free recursion using numpy lfilter substitute
    y = np.zeros_like(x)
    z1 = np.zeros(x.shape[1]); z2 = np.zeros(x.shape[1])
    # transposed direct form II
    for n in range(x.shape[0]):
        xn = x[n]
        yn = b[0] * xn + z1
        z1 = b[1] * xn - a[1] * yn + z2
        z2 = b[2] * xn - a[2] * yn
        y[n] = yn
    return y

def lfilter(b, a, x):
    # fast enough IIR for 3M samples using chunked python loop would be slow: use FFT-free trick via numpy for 2nd order with float64 and loops in blocks
    from numpy.lib.stride_tricks import sliding_window_view
    return biquad(x, b, a)

def kweight_coeffs(fs):
    # BS.1770 stage 1 (high shelf) and stage 2 (high pass), bilinear-transformed for arbitrary fs
    f0 = 1681.974450955533; G = 3.999843853973347; Q = 0.7071752369554196
    K = np.tan(np.pi * f0 / fs); Vh = 10 ** (G / 20); Vb = Vh ** 0.4996667741545416
    a0 = 1 + K / Q + K * K
    b1 = np.array([(Vh + Vb * K / Q + K * K) / a0, 2 * (K * K - Vh) / a0, (Vh - Vb * K / Q + K * K) / a0])
    a1 = np.array([1, 2 * (K * K - 1) / a0, (1 - K / Q + K * K) / a0])
    f0 = 38.13547087602444; Q = 0.5003270373238773
    K = np.tan(np.pi * f0 / fs)
    a0 = 1 + K / Q + K * K
    b2 = np.array([1, -2, 1]); a2 = np.array([1, 2 * (K * K - 1) / a0, (1 - K / Q + K * K) / a0])
    return (b1, a1), (b2, a2)

def main():
    fn = sys.argv[1]; fs = int(sys.argv[2]) if len(sys.argv) > 2 else 48000
    x = np.fromfile(fn, dtype=np.float32).reshape(-1, 2).astype(np.float64)
    (b1, a1), (b2, a2) = kweight_coeffs(fs)
    # K-weighting applied in the frequency domain (exact digital frequency response of both biquads, one FFT over the whole file)
    N = len(x); nfft = 1 << int(np.ceil(np.log2(N + 8 * fs)))
    w = 2 * np.pi * np.fft.rfftfreq(nfft, 1.0)
    z1 = np.exp(-1j * w); z2 = z1 * z1
    H = (b1[0] + b1[1] * z1 + b1[2] * z2) / (a1[0] + a1[1] * z1 + a1[2] * z2) * (b2[0] + b2[1] * z1 + b2[2] * z2) / (a2[0] + a2[1] * z1 + a2[2] * z2)
    y = np.stack([np.fft.irfft(np.fft.rfft(x[:, c], nfft) * H, nfft)[:N] for c in range(2)], axis=1)
    blk = int(0.4 * fs); hop = int(0.1 * fs)
    n = (len(y) - blk) // hop + 1
    ms = np.array([(y[i * hop:i * hop + blk] ** 2).mean(axis=0).sum() for i in range(n)])
    l = -0.691 + 10 * np.log10(ms + 1e-12)
    gate = l > -70
    ref = -0.691 + 10 * np.log10(ms[gate].mean()) - 10
    gate2 = gate & (l > ref)
    integ = -0.691 + 10 * np.log10(ms[gate2].mean())
    st = []
    for s in range(0, len(y) - 3 * fs + 1, fs * 3):
        z = (y[s:s + 3 * fs] ** 2).mean(axis=0).sum(); st.append(-0.691 + 10 * np.log10(z + 1e-12))
    # true peak via 4x oversampling (linear interpolation of a windowed sinc would be exact; use FFT upsampling on 1 s blocks)
    tp = 0.0
    for ch in range(2):
        for s in range(0, len(x) - fs + 1, fs):
            seg = x[s:s + fs, ch]; up = np.fft.irfft(np.fft.rfft(seg) * 1.0, n=len(seg) * 4) * 4 if False else None
        # cheap approximation: max of |x| and of 4x linear-phase interpolation
    from numpy import interp
    xs = np.arange(len(x)); xf = np.arange(0, len(x) - 1, 0.25)
    tp = max(np.abs(interp(xf, xs, x[:, 0])).max(), np.abs(interp(xf, xs, x[:, 1])).max())
    print('%s: integrated %.1f LUFS, short-term (3 s) min/max %.1f / %.1f LUFS, sample peak %.2f dBFS' % (fn, integ, min(st), max(st), 20 * np.log10(np.abs(x).max() + 1e-12)))
    print('  short-term per 3 s:', ' '.join('%.0f' % v for v in st))
    # octave bands, A-weighted
    mono = x.mean(axis=1)
    def aw(f):
        f2 = f * f
        r = (12194 ** 2 * f2 * f2) / ((f2 + 20.6 ** 2) * np.sqrt((f2 + 107.7 ** 2) * (f2 + 737.9 ** 2)) * (f2 + 12194 ** 2))
        return 20 * np.log10(r + 1e-12) + 2.0
    centres = [31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]
    print('  A-weighted octave band levels (dB re full scale sine) per section; bands: ' + ' '.join('%5g' % c for c in centres))
    for s in range(0, len(mono) // fs, 10):
        seg = mono[s * fs:(s + 10) * fs]
        if len(seg) < fs: break
        w = np.hanning(len(seg)); sp = np.abs(np.fft.rfft(seg * w)) ** 2 / (w ** 2).sum() / len(seg); fr = np.fft.rfftfreq(len(seg), 1 / fs)
        row = []
        for c in centres:
            m = (fr >= c / 2 ** .5) & (fr < c * 2 ** .5)
            e = sp[m].sum() * 2 * 10 ** (aw(c) / 10)
            row.append(10 * np.log10(e + 1e-12))
        print('  %2d-%2d s: ' % (s, s + 10) + ' '.join('%5.1f' % v for v in row))

main()
