/* packer: compresses a file with the model in model.h.
 * Searches the context mask set and the mixer parameters for the smallest
 * output, then writes: u32 len, u8 nm, u8 lrate, u8 climit, nm mask bytes,
 * arithmetic-coded stream.
 *
 * usage: packer in out [maxmodels]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "model.h"

static u8 *inb;
static int inlen;

static void reset(void)
{
    memset(Pt, 0, sizeof Pt);
    memset(Nt, 0, sizeof Nt);
    memset(hist, 0, sizeof hist);
    model_init();
}

/* estimated compressed size in bits */
static double cost(void)
{
    reset();
    double bits = 0;
    for (int k = 0; k < inlen; k++) {
        int c = inb[k];
        model_byte();
        u32 c0 = 1;
        for (int bp = 0; bp < 8; bp++) {
            int y = c >> (7 - bp) & 1;
            int p = model_predict(c0, bp);
            bits -= log2((y ? p : 4096 - p) / 4096.0);
            model_update(y, p, bp);
            c0 = c0 * 2 + y;
        }
        model_push(c);
    }
    return bits;
}

static u8 outb[1 << 22];
static int outn;

static int encode(void)
{
    reset();
    outn = 0;
    u32 x1 = 0, x2 = 0xffffffffu;
    for (int k = 0; k < inlen; k++) {
        int c = inb[k];
        model_byte();
        u32 c0 = 1;
        for (int bp = 0; bp < 8; bp++) {
            int y = c >> (7 - bp) & 1;
            int p = model_predict(c0, bp);
            u32 xmid = x1 + ((x2 - x1) >> 12) * p;
            if (y) x2 = xmid; else x1 = xmid + 1;
            while (((x1 ^ x2) & 0xff000000u) == 0) {
                outb[outn++] = x2 >> 24;
                x1 <<= 8;
                x2 = (x2 << 8) | 255;
            }
            model_update(y, p, bp);
            c0 = c0 * 2 + y;
        }
        model_push(c);
    }
    for (int i = 0; i < 4; i++) { outb[outn++] = x1 >> 24; x1 <<= 8; }
    return outn;
}

/* reference decoder, to verify the stream round-trips */
static int verify(const u8 *s)
{
    reset();
    u32 x1 = 0, x2 = 0xffffffffu, x = 0;
    int pos = 0;
    for (int i = 0; i < 4; i++) x = (x << 8) | s[pos++];
    for (int k = 0; k < inlen; k++) {
        model_byte();
        u32 c0 = 1;
        for (int bp = 0; bp < 8; bp++) {
            int p = model_predict(c0, bp);
            u32 xmid = x1 + ((x2 - x1) >> 12) * p;
            int y = x <= xmid;
            if (y) x2 = xmid; else x1 = xmid + 1;
            while (((x1 ^ x2) & 0xff000000u) == 0) {
                x1 <<= 8;
                x2 = (x2 << 8) | 255;
                x = (x << 8) | (pos < outn ? s[pos] : 0);
                pos++;
            }
            model_update(y, p, bp);
            c0 = c0 * 2 + y;
        }
        if ((int)(c0 & 255) != inb[k]) return 0;
        model_push(c0 & 255);
    }
    return 1;
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: packer in out [maxmodels]\n"); return 1; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    inb = malloc(1 << 22);
    inlen = fread(inb, 1, 1 << 22, f);
    fclose(f);
    int maxm = argc > 3 ? atoi(argv[3]) : 8;
    if (maxm > MAXM) maxm = MAXM;

    /* start: orders 0..4 */
    static const u8 init[] = {0x00, 0x01, 0x03, 0x07, 0x0f};
    nm = 5;
    memcpy(masks, init, nm);
    lrate = 6; climit = 60;
    double best = cost();
    fprintf(stderr, "packer: %d bytes, initial %.0f bytes\n", inlen, best / 8);

    for (int round = 0; round < 40; round++) {
        int improved = 0;
        /* try adding a mask */
        if (nm < maxm) {
            int bm = -1;
            double bc = best;
            for (int m = 0; m < 256; m++) {
                int dup = 0;
                for (int i = 0; i < nm; i++) dup |= masks[i] == m;
                if (dup) continue;
                masks[nm++] = m;
                double c = cost();
                nm--;
                if (c < bc - 8) { bc = c; bm = m; }
            }
            if (bm >= 0) { masks[nm++] = bm; best = bc; improved = 1; }
        }
        /* try replacing / removing masks */
        for (int i = 0; i < nm; i++) {
            u8 old = masks[i];
            int bm = -1;
            double bc = best;
            for (int m = 0; m < 256; m++) {
                int dup = 0;
                for (int j = 0; j < nm; j++) dup |= masks[j] == m;
                if (dup) continue;
                masks[i] = m;
                double c = cost();
                if (c < bc - 1) { bc = c; bm = m; }
            }
            masks[i] = bm >= 0 ? bm : old;
            if (bm >= 0) { best = bc; improved = 1; }
        }
        for (int i = 0; i < nm && nm > 1; i++) {
            u8 save[MAXM];
            memcpy(save, masks, nm);
            memmove(masks + i, masks + i + 1, nm - i - 1);
            nm--;
            double c = cost();
            if (c < best) { best = c; improved = 1; i--; }
            else { nm++; memcpy(masks, save, nm); }
        }
        /* parameters */
        for (int lr = 1; lr <= 24; lr++) {
            int o = lrate; lrate = lr;
            double c = cost();
            if (c < best) { best = c; improved = 1; } else lrate = o;
        }
        static const int lims[] = {3, 7, 15, 30, 60, 120, 250};
        for (int k = 0; k < 7; k++) {
            int o = climit; climit = lims[k];
            double c = cost();
            if (c < best) { best = c; improved = 1; } else climit = o;
        }
        fprintf(stderr, "  round %d: %.0f bytes, %d models, lr %d, lim %d\n",
                round, best / 8, nm, lrate, climit);
        if (!improved) break;
    }

    int n = encode();
    static u8 copy[1 << 22];
    memcpy(copy, outb, n);
    if (!verify(copy)) { fprintf(stderr, "packer: VERIFY FAILED\n"); return 1; }

    f = fopen(argv[2], "wb");
    u8 hdr[7 + MAXM];
    hdr[0] = inlen; hdr[1] = inlen >> 8; hdr[2] = inlen >> 16; hdr[3] = inlen >> 24;
    hdr[4] = nm; hdr[5] = lrate; hdr[6] = climit;
    memcpy(hdr + 7, masks, nm);
    fwrite(hdr, 1, 7 + nm, f);
    fwrite(copy, 1, n, f);
    fclose(f);
    fprintf(stderr, "packer: %d -> %d bytes (+%d header), masks:", inlen, n, 7 + nm);
    for (int i = 0; i < nm; i++) fprintf(stderr, " %02x", masks[i]);
    fprintf(stderr, "\n");
    return 0;
}
