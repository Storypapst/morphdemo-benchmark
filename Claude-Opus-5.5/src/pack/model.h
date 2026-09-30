/* Context-mixing bit model shared by the packer (host) and the unpacker stub.
 * Integer-only so encoder and decoder agree bit for bit.
 *
 * Every model hashes a subset of the previous 8 bytes (selected by a mask
 * byte, chosen per file by the packer) together with the partial current
 * byte. Each slot holds a 16-bit probability and a hit count (adaptive
 * learning rate). Predictions are combined by a logistic mixer whose weight
 * set is selected by the bit position.
 */
#ifndef MODEL_H
#define MODEL_H

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define TBITS 22
#define MAXM 12

static short Pt[1 << TBITS];   /* probability - 32768 (so zeroed = 0.5) */
static u8 Nt[1 << TBITS];      /* hit count */
static short strt[4096];       /* stretch table */
static int Wt[8][MAXM + 1];    /* mixer weights per bit position */
static u32 Hc[MAXM];           /* per-model hash at byte boundary */
static u32 Ix[MAXM];           /* per-model slot of current bit */
static int Xs[MAXM + 1];       /* stretched inputs */
static u8 hist[8];             /* previous bytes, hist[0] = most recent */

static const short sqt[33] = {1,2,3,6,10,16,27,45,73,120,194,310,488,747,1101,
  1546,2047,2549,2994,3348,3607,3785,3901,3975,4022,4050,4068,4079,4085,4089,
  4092,4093,4094};

static int squash(int d)
{
    if (d > 2047) d = 2047;
    if (d < -2047) d = -2047;
    int w = d & 127;
    d = (d >> 7) + 16;
    return (sqt[d] * (128 - w) + sqt[d + 1] * w + 64) >> 7;
}

/* model parameters (from the stream header) */
static int nm;
static u8 masks[MAXM];
static int lrate, climit;

static void model_init(void)
{
    int pi = 0;
    for (int x = -2047; x <= 2047; x++) {
        int v = squash(x);
        while (pi <= v) strt[pi++] = x;
    }
    while (pi < 4096) strt[pi++] = 2047;
    for (int b = 0; b < 8; b++)
        for (int i = 0; i <= nm; i++) Wt[b][i] = 1 << 14;
}

static void model_byte(void)
{
    for (int i = 0; i < nm; i++) {
        u32 h = masks[i] * 0x3D4D51CBu;
        for (int j = 0; j < 8; j++)
            if (masks[i] >> j & 1) h = (h + hist[j] + 1) * 0x2F0B4C1Du;
        Hc[i] = h;
    }
}

/* c0 = partial byte with leading 1, bp = bit position 0..7 */
static int model_predict(u32 c0, int bp)
{
    int dot = 0;
    int *w = Wt[bp];
    for (int i = 0; i < nm; i++) {
        u32 ix = ((Hc[i] ^ (c0 * 0x6F4F2A35u)) * 0x9E3779B1u) >> (32 - TBITS);
        Ix[i] = ix;
        Xs[i] = strt[(Pt[ix] + 32768) >> 4];
    }
    Xs[nm] = 256;
    for (int i = 0; i <= nm; i++) dot += (int)(((long long)Xs[i] * w[i]) >> 16);
    int p = squash(dot);
    return p < 1 ? 1 : p > 4095 ? 4095 : p;
}

static void model_update(int y, int p, int bp)
{
    int err = ((y << 12) - p) * lrate;
    int *w = Wt[bp];
    for (int i = 0; i <= nm; i++) w[i] += (Xs[i] * err) >> 10;
    for (int i = 0; i < nm; i++) {
        u32 ix = Ix[i];
        int n = Nt[ix];
        int pv = Pt[ix] + 32768;
        pv += (((y << 16) - y - pv) * 2) / (2 * n + 3);
        Pt[ix] = pv - 32768;
        if (n < climit) Nt[ix] = n + 1;
    }
}

static void model_push(int c)
{
    for (int j = 7; j > 0; j--) hist[j] = hist[j - 1];
    hist[0] = c;
}

#endif
