// Audio path latency probe (development only): queue a click after 0.5 s of silence, unpause, print the monotonic time of the unpause call.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
typedef struct { int freq; uint16_t format; uint8_t channels, silence; uint16_t samples, padding; uint32_t size; void *callback; void *userdata; } SDL_AudioSpec;
int SDL_Init(uint32_t);
uint32_t SDL_OpenAudioDevice(const char*, int, const SDL_AudioSpec*, SDL_AudioSpec*, int);
int SDL_QueueAudio(uint32_t, const void*, uint32_t);
void SDL_PauseAudioDevice(uint32_t,int);
void SDL_Delay(uint32_t);
static double now(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return ts.tv_sec+ts.tv_nsec*1e-9; }
int main(int argc,char**argv){
  int samples=argc>1?atoi(argv[1]):0; double at=argc>2?atof(argv[2]):0.5;
  SDL_Init(0x10);
  static float buf[48000*2*3];
  int c=(int)(at*48000);
  for(int i=0;i<480;i++){ buf[2*(c+i)]=buf[2*(c+i)+1]=0.5f*((i&1)?1.f:-1.f); }   // 10 ms of a 24 kHz square: easy to locate
  SDL_AudioSpec want; memset(&want,0,sizeof want); want.freq=48000; want.format=0x8120; want.channels=2; want.samples=samples;
  SDL_AudioSpec got; memset(&got,0,sizeof got);
  uint32_t dev=SDL_OpenAudioDevice(0,0,&want,&got,0);
  fprintf(stderr,"opened dev=%u samples asked=%d got=%d\n",dev,samples,got.samples);
  SDL_QueueAudio(dev,buf,sizeof buf);
  double t=now(); SDL_PauseAudioDevice(dev,0);
  printf("%.6f %.6f\n",t,at);
  fflush(stdout);
  SDL_Delay(2200);
  return 0;
}
