// morphdemo-16k: window, GPU-computed audio, HDR particle renderer with mip-chain bloom.
#define IMPORTS(X) \
 X(int,SDL_Init,(u32)) \
 X(void*,SDL_CreateWindow,(const char*,int,int,int,int,u32)) \
 X(void*,SDL_GL_CreateContext,(void*)) \
 X(int,SDL_GL_SetSwapInterval,(int)) \
 X(u32,SDL_OpenAudioDevice,(const char*,int,const void*,void*,int)) \
 X(int,SDL_QueueAudio,(u32,const void*,u32)) \
 X(void,SDL_PauseAudioDevice,(u32,int)) \
 X(u32,SDL_GetTicks,(void)) \
 X(int,SDL_PollEvent,(void*)) \
 X(void,SDL_GL_SwapWindow,(void*)) \
 X(u32,glCreateShaderProgramv,(u32,int,const char*const*)) \
 X(u32,glCreateProgram,(void)) \
 X(u32,glCreateShader,(u32)) \
 X(void,glShaderSource,(u32,int,const char*const*,const int*)) \
 X(void,glCompileShader,(u32)) \
 X(void,glAttachShader,(u32,u32)) \
 X(void,glLinkProgram,(u32)) \
 X(void,glUseProgram,(u32)) \
 X(void,glBindBufferBase,(u32,u32,u32)) \
 X(void,glBufferData,(u32,long,const void*,u32)) \
 X(void,glDispatchCompute,(u32,u32,u32)) \
 X(void*,glMapBuffer,(u32,u32)) \
 X(void,glUniform3i,(int,int,int,int)) \
 X(void,glRects,(short,short,short,short)) \
 X(void,glGenTextures,(int,u32*)) \
 X(void,glBindTexture,(u32,u32)) \
 X(void,glTexImage2D,(u32,int,int,int,int,int,u32,u32,const void*)) \
 X(void,glTexParameteri,(u32,u32,int)) \
 X(void,glGenFramebuffers,(int,u32*)) \
 X(void,glBindFramebuffer,(u32,u32)) \
 X(void,glFramebufferTexture2D,(u32,u32,u32,u32,int)) \
 X(void,glGenerateMipmap,(u32)) \
 X(void,glEnable,(u32)) \
 X(void,glDisable,(u32)) \
 X(void,glBlendFunc,(u32,u32)) \
 X(void,glClear,(u32)) \
 X(void,glDrawArrays,(u32,int,int)) \
 X(void,glUniform1i,(int,int)) \
 X(void,glMemoryBarrier,(u32)) \
 X(void,glViewport,(int,int,int,int)) \
 X(int,SDL_ShowCursor,(int)) \
 X(void,SDL_GL_GetDrawableSize,(void*,int*,int*)) \
 X(void,glBindBuffer,(u32,u32)) \
 X(void,glEnableVertexAttribArray,(u32)) \
 X(void,glVertexAttribPointer,(u32,int,u32,u8,int,const void*))
#include "../common/crt.h"

#define NSAMP 2880000
#define NPART ((1 << 20) + 100000 + 200000)
EMBED(vs_src, "build/16k/pts.vert.min");
EMBED(fs_src, "build/16k/pts.frag.min");
EMBED(post_src, "build/16k/post.frag.min");
EMBED(aud_src, "build/16k/aud.min");
EMBED(rev_src, "build/16k/rev.min");

static u32 mkprog(const char *vs, const char *fs) {
  u32 p = CALL(glCreateProgram);
  u32 a = CALL(glCreateShader, 0x8B31), b = CALL(glCreateShader, 0x8B30);
  CALL(glShaderSource, a, 1, &vs, 0); CALL(glCompileShader, a); CALL(glAttachShader, p, a);
  CALL(glShaderSource, b, 1, &fs, 0); CALL(glCompileShader, b); CALL(glAttachShader, p, b);
  CALL(glLinkProgram, p);
  return p;
}

void __attribute__((section(".text.entry"), used)) DEMO_ENTRY(void) {
  crt_resolve();
  crt_alarm(67);                                       // watchdog: never run much longer than the production
  CALL(SDL_Init, 0x30);
  void *win = CALL(SDL_CreateWindow, "MORPH", 0x1fff0000, 0x1fff0000, 0, 0, 0x1003);
  CALL(SDL_GL_CreateContext, win);
  CALL(SDL_GL_SetSwapInterval, 1);
  CALL(SDL_ShowCursor, 0);
  // audio, two compute passes: 1) dry mix + reverb send per sample, 2) convolution reverb and master
  const char *s = aud_src;
  u32 pa = CALL(glCreateShaderProgramv, 0x91B9, 1, &s);
  s = rev_src;
  u32 pr = CALL(glCreateShaderProgramv, 0x91B9, 1, &s);
  CALL(glBindBufferBase, 0x90D2, 0, 1);
  CALL(glBufferData, 0x90D2, (long)NSAMP * 16, 0, 0x88E9);
  CALL(glBindBufferBase, 0x90D2, 1, 2);
  CALL(glBufferData, 0x90D2, (long)NSAMP * 8, 0, 0x88E9);
  CALL(glUseProgram, pa);
  CALL(glDispatchCompute, NSAMP / 64, 1, 1);
  CALL(glMemoryBarrier, 0x2000);
  CALL(glUseProgram, pr);
  for (int b = 0; b < NSAMP; b += NSAMP / 20) { CALL(glUniform1i, 1, b); CALL(glDispatchCompute, NSAMP / 20 / 64, 1, 1); }
  CALL(glMemoryBarrier, 0x200);                        // GL_BUFFER_UPDATE_BARRIER_BIT: the shader writes must be visible to the mapping
  void *pcm = CALL(glMapBuffer, 0x90D2, 0x88B8);
  struct { int freq; u16 fmt; u8 ch, sil; u16 samples, pad; u32 size; void *cb, *ud; } spec = { 48000, 0x8120, 2, 0, 0, 0, 0, 0, 0 };
  u32 dev = CALL(SDL_OpenAudioDevice, 0, 0, &spec, 0, 0);
  CALL(SDL_QueueAudio, dev, pcm, NSAMP * 8);
  // graphics
  u32 pp = mkprog(vs_src, fs_src);
  u32 post = CALL(glCreateShaderProgramv, 0x8B30, 1, &(const char *){post_src});
  u32 tex, fbo;
  CALL(glGenTextures, 1, &tex);
  CALL(glBindTexture, 0x0DE1, tex);
  CALL(glTexParameteri, 0x0DE1, 0x2801, 0x2703);
  CALL(glTexParameteri, 0x0DE1, 0x2800, 0x2601);
  CALL(glTexParameteri, 0x0DE1, 0x2802, 0x812F);
  CALL(glTexParameteri, 0x0DE1, 0x2803, 0x812F);
  CALL(glGenFramebuffers, 1, &fbo);
  // the compatibility profile only issues vertices while attribute 0 is enabled: point it at the (unused) audio buffer
  CALL(glBindBuffer, 0x8892, 1);
  CALL(glEnableVertexAttribArray, 0);
  CALL(glVertexAttribPointer, 0, 1, 0x1401, 0, 0, 0);
  int W = 0, H = 0;
  u32 t0 = 0;
  for (int first = 1;; first = 0) {
    if (!first && !t0) { CALL(SDL_PauseAudioDevice, dev, 0); t0 = CALL(SDL_GetTicks); }   // the first frame only warms up the shaders
    u32 t = t0 ? CALL(SDL_GetTicks) - t0 : 0;
    if (t >= 60000) break;
    u32 ev[14];
    while (CALL(SDL_PollEvent, ev)) if (ev[0] == 0x100 || (ev[0] == 0x300 && ev[5] == 27)) crt_exit(0);
    int w, h; CALL(SDL_GL_GetDrawableSize, win, &w, &h);
    CALL(glBindFramebuffer, 0x8D40, fbo);
    if (w != W || h != H) {                              // first frame, or the window changed size: rebuild the float target
      W = w; H = h;
      CALL(glTexImage2D, 0x0DE1, 0, 0x881A, W, H, 0, 0x1908, 0x140B, 0);
      CALL(glFramebufferTexture2D, 0x8D40, 0x8CE0, 0x0DE1, tex, 0);
    }
    // pass 1: particles, additive, into the float target
    CALL(glViewport, 0, 0, W, H);
    CALL(glClear, 0x4000);
    CALL(glEnable, 0x0BE2); CALL(glBlendFunc, 1, 1); CALL(glEnable, 0x8642); CALL(glEnable, 0x8861);
    CALL(glUseProgram, pp);
    CALL(glUniform3i, 0, t, W, H);
    CALL(glDrawArrays, 0, 0, NPART);
    CALL(glDisable, 0x0BE2);
    // pass 2: bloom from the mip chain, tone map, grain
    CALL(glBindFramebuffer, 0x8D40, 0);
    CALL(glGenerateMipmap, 0x0DE1);
    CALL(glUseProgram, post);
    CALL(glUniform3i, 0, t, W, H);
    CALL(glRects, -1, -1, 1, 1);
    CALL(SDL_GL_SwapWindow, win);
  }
  crt_exit(0);
}
#ifdef HOST
int main(void) { demo_main(); return 0; }
#endif
