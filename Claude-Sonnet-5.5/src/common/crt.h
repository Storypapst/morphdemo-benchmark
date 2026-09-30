// Minimal freestanding runtime for the C payloads (16k, 64k).
// No libc: every library function is resolved through dlsym at start-up. A demo declares its imports once,
//     #define IMPORTS(X) X(int,SDL_Init,(u32)) X(...)
//     #include "crt.h"
// and afterwards calls them with CALL(SDL_Init, 0x30).
// Compiled with -DHOST the same source builds as an ordinary dynamically linked program (development builds).
#ifndef CRT_H
#define CRT_H
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef unsigned long long u64;
typedef signed char s8; typedef short s16; typedef int s32; typedef long long s64;

#ifdef HOST
#define X(r,n,a) extern r n a;
IMPORTS(X)
#undef X
#define CALL(n, ...) n(__VA_ARGS__)
static void crt_resolve(void) {}
#define DEMO_ENTRY demo_main
static inline void crt_exit(int code) { extern void _exit(int) __attribute__((noreturn)); _exit(code); }
#else
#define GOT_DLSYM (*(void *(**)(void *, const char *))0x400028)   // filled by ld.so (relocation in the ELF stub)
enum {
#define X(r,n,a) I_##n,
IMPORTS(X)
#undef X
  I_COUNT
};
static void *PT[I_COUNT];
#define X(r,n,a) typedef r (*T_##n)a;
IMPORTS(X)
#undef X
static const char IMPORT_NAMES[] =
#define X(r,n,a) #n "\0"
IMPORTS(X)
#undef X
;
#define CALL(n, ...) ((T_##n)PT[I_##n])(__VA_ARGS__)
static void crt_resolve(void) {
  const char *s = IMPORT_NAMES;
  for (int i = 0; i < I_COUNT; i++) { PT[i] = GOT_DLSYM(0, s); while (*s++) ; }
}
#define DEMO_ENTRY entry
static inline void crt_exit(int code) { __asm__ volatile("syscall" :: "a"(231), "D"(code) : "rcx", "r11", "memory"); __builtin_unreachable(); }
#endif

static inline void crt_alarm(int sec) {
#ifdef HOST
  extern unsigned alarm(unsigned); alarm(sec);
#else
  long r; __asm__ volatile("syscall" : "=a"(r) : "a"(37), "D"(sec) : "rcx", "r11", "memory");
#endif
}

#ifdef DBG
static inline void dbg(const char *m) { long r; __asm__ volatile("syscall" : "=a"(r) : "a"(1), "D"(2), "S"(m), "d"(2) : "rcx", "r11", "memory"); }
#else
#define dbg(m) ((void)0)
#endif

// data embedded from files (GLSL etc.): EMBED(symbol, "path") -> const char symbol[] (NUL terminated)
#define EMBED(sym, path) __asm__(".section .rodata\n.global " #sym "\n" #sym ":\n.incbin \"" path "\"\n.byte 0\n.previous\n"); extern const char sym[]
#endif
