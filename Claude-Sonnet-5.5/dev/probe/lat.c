#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
typedef struct { int freq; uint16_t format; uint8_t channels, silence; uint16_t samples, padding; uint32_t size; void *callback; void *userdata; } SDL_AudioSpec;
int SDL_Init(uint32_t);
uint32_t SDL_OpenAudioDevice(const char*, int, const SDL_AudioSpec*, SDL_AudioSpec*, int);
int SDL_QueueAudio(uint32_t, const void*, uint32_t);
uint32_t SDL_GetQueuedAudioSize(uint32_t);
void SDL_PauseAudioDevice(uint32_t,int);
void SDL_Delay(uint32_t);
const char *SDL_GetError(void);
static double now(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return ts.tv_sec+ts.tv_nsec*1e-9; }
int main(int argc,char**argv){
  setvbuf(stdout,0,_IONBF,0);
  int samples = argc>1?atoi(argv[1]):0;
  SDL_Init(0x10);
  static float buf[48000*2*3];
  SDL_AudioSpec want; memset(&want,0,sizeof want); want.freq=48000; want.format=0x8120; want.channels=2; want.samples=samples;
  SDL_AudioSpec got; memset(&got,0,sizeof got);
  double t0=now();
  uint32_t dev=SDL_OpenAudioDevice(0,0,&want,&got,0);
  printf("open dev=%u after %.1f ms; got freq=%d fmt=%x ch=%d samples=%d\n",dev,(now()-t0)*1e3,got.freq,got.format,got.channels,got.samples);
  uint32_t total=sizeof buf; SDL_QueueAudio(dev,buf,total);
  double t1=now(); SDL_PauseAudioDevice(dev,0);
  uint32_t q; int steps=0; double first=-1, last=t1; uint32_t prev=total;
  while(now()-t1<1.5){ q=SDL_GetQueuedAudioSize(dev); if(q!=prev){ if(first<0) first=now()-t1; steps++; if(steps<=6) printf("  +%.1f ms queued %u (consumed %u)\n",(now()-t1)*1e3,q,total-q); prev=q; } }
  printf("first consumption after unpause: %.1f ms, steps in 1.5s: %d, consumed %u bytes = %.3f s\n", first*1e3, steps, total-prev, (total-prev)/(48000.0*8));
  return 0;
}
