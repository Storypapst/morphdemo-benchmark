/* Unpacker stub: decodes the appended stream into memory, writes it to an
 * anonymous memfd and executes that. No libc, no dynamic linking. */
#include "model.h"

extern u8 blob[];
static u8 out[1 << 21];

static long sys6(long n, long a, long b, long c, long d, long e)
{
    long r;
    register long r10 __asm__("r10") = d;
    register long r8 __asm__("r8") = e;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8)
                     : "rcx", "r11", "memory");
    return r;
}

__attribute__((noreturn, used)) void entry(long *sp)
{
    u8 *s = blob;
    u32 len = *(u32 *)s;
    nm = s[4];
    lrate = s[5];
    climit = s[6];
    for (int i = 0; i < nm; i++) masks[i] = s[7 + i];
    s += 7 + nm;
    model_init();
    u32 x1 = 0, x2 = 0xffffffffu, x = 0;
    for (int i = 0; i < 4; i++) x = (x << 8) | *s++;
    for (u32 k = 0; k < len; k++) {
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
                x = (x << 8) | *s++;
            }
            model_update(y, p, bp);
            c0 = c0 * 2 + y;
        }
        out[k] = c0;
        model_push(c0 & 255);
    }
    long fd = sys6(319, (long)"", 0x10, 0, 0, 0);           /* memfd_create, MFD_EXEC */
    sys6(1, fd, (long)out, len, 0, 0);                       /* write */
    char **argv = (char **)(sp + 1);
    sys6(322, fd, (long)"", (long)argv, (long)(argv + sp[0] + 1), 0x1000); /* execveat, AT_EMPTY_PATH */
    for (;;) sys6(231, 1, 0, 0, 0, 0);                       /* exit_group */
}
