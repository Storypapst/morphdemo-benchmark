// Environment probe: SDL2 window + GL context + audio device. Not part of the deliverables.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
typedef struct { int freq; uint16_t format; uint8_t channels, silence; uint16_t samples, padding; uint32_t size; void (*callback)(void*, uint8_t*, int); void *userdata; } SDL_AudioSpec;
int SDL_Init(uint32_t);
void *SDL_CreateWindow(const char*, int, int, int, int, uint32_t);
void *SDL_GL_CreateContext(void*);
void *SDL_GL_GetProcAddress(const char*);
void SDL_GL_SwapWindow(void*);
void SDL_GL_GetDrawableSize(void*, int*, int*);
void SDL_GetWindowSize(void*, int*, int*);
int SDL_GL_SetSwapInterval(int);
int SDL_PollEvent(void*);
uint32_t SDL_GetTicks(void);
const char *SDL_GetCurrentVideoDriver(void);
const char *SDL_GetCurrentAudioDriver(void);
int SDL_OpenAudio(SDL_AudioSpec*, SDL_AudioSpec*);
void SDL_PauseAudio(int);
void SDL_Quit(void);
const char *SDL_GetError(void);
int SDL_ShowCursor(int);
int SDL_GL_GetAttribute(int, int*);
void SDL_GetVersion(uint8_t*);

static double now(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return ts.tv_sec+ts.tv_nsec*1e-9; }
static uint64_t samples_done;
static void cb(void *u, uint8_t *s, int n){ int16_t *o=(int16_t*)s; for(int i=0;i<n/4;i++){ float ph=(float)(samples_done++)/48000.0f; int16_t v=(int16_t)(((int)(ph*440*2)&1?1:-1)*2000); o[2*i]=v;o[2*i+1]=v; } }

int main(void){
  setvbuf(stdout,0,_IONBF,0); double t0=now();
  if(SDL_Init(0x30)){ printf("init fail %s\n",SDL_GetError()); return 1; }
  printf("SDL_Init %.3fs, video=%s audio=%s\n", now()-t0, SDL_GetCurrentVideoDriver(), SDL_GetCurrentAudioDriver());
  uint8_t v[3]; SDL_GetVersion(v); printf("SDL version %d.%d.%d\n", v[0],v[1],v[2]);
  void *w=SDL_CreateWindow("probe",0x1FFF0000,0x1FFF0000,1920,1080,0x1003);
  printf("window %p %.3fs %s\n", w, now()-t0, w?"":SDL_GetError());
  void *c=SDL_GL_CreateContext(w);
  printf("ctx %p %.3fs %s\n", c, now()-t0, c?"":SDL_GetError());
  const char *(*glGetString)(unsigned)=SDL_GL_GetProcAddress("glGetString");
  void (*glClearColor)(float,float,float,float)=SDL_GL_GetProcAddress("glClearColor");
  void (*glClear)(unsigned)=SDL_GL_GetProcAddress("glClear");
  printf("GL_VERSION %s\nGL_RENDERER %s\n", glGetString(0x1F02), glGetString(0x1F01));
  int prof=-1,maj=-1,min=-1; SDL_GL_GetAttribute(21,&prof); SDL_GL_GetAttribute(17,&maj); SDL_GL_GetAttribute(18,&min); printf("profile mask %d major %d minor %d\n",prof,maj,min);
  int dw,dh,ww,wh; SDL_GL_GetDrawableSize(w,&dw,&dh); SDL_GetWindowSize(w,&ww,&wh); printf("drawable %dx%d window %dx%d\n",dw,dh,ww,wh);
  SDL_ShowCursor(0); if(getenv("SWAP")) printf("swapint=%d\n", SDL_GL_SetSwapInterval(atoi(getenv("SWAP"))));
  SDL_AudioSpec want; memset(&want,0,sizeof want); want.freq=44100; want.format=0x8010; want.channels=2; want.samples=1024; want.callback=cb;
  SDL_AudioSpec got; int r=SDL_OpenAudio(&want,&got); printf("audio open r=%d freq=%d fmt=%x ch=%d samples=%d %s\n", r, got.freq, got.format, got.channels, got.samples, r?SDL_GetError():"");
  SDL_PauseAudio(0);
  double ts=now(); int frames=0; uint8_t ev[64];
  while(now()-ts<3.0){
    float f=(now()-ts)/3.0f; glClearColor(f,0.2f,1-f,1); glClear(0x4000); SDL_GL_SwapWindow(w); frames++;
    while(SDL_PollEvent(ev)){ uint32_t ty=*(uint32_t*)ev; if(ty==0x300){ printf("key %d\n", *(int*)(ev+20)); } }
  }
  printf("frames %d in 3s (%.1f fps) audio samples produced %llu\n", frames, frames/3.0, (unsigned long long)samples_done);
  SDL_Quit();
  return 0;
}
