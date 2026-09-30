/* Development-only interposer. Never linked into a final demo.
 * Captures actual GL frames and the actual samples passed to SDL, measures
 * frame cadence, and optionally injects Escape through the normal event API. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

typedef struct {
 int freq; uint16_t format; uint8_t channels,silence; uint16_t samples,padding;
 uint32_t size; void (*callback)(void *, uint8_t *, int); void *userdata;
} AudioSpec;
static void (*audio_cb)(void *,uint8_t *,int);
static void *audio_ud;
static FILE *samples_file;
static char folder[4096];
static double first, last, max_gap, sum_gap, audio_energy, audio_peak;
static uint64_t frames, samples_count, clipped;
static int finished, escape_sent, next_shot, audio_rate, audio_channels, audio_format;
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static void init(void) {
 if(folder[0]) return;
 const char *v=getenv("MORPH_AUDIT_DIR"); if(!v) return;
 snprintf(folder,sizeof folder,"%s",v);
}
static void save_audio(const uint8_t *buf,int n) {
 init(); if(!folder[0])return;
 if(!samples_file){ char name[4200];snprintf(name,sizeof name,"%s/audio.f32",folder); samples_file=fopen(name,"wb"); }
 if(samples_file)fwrite(buf,1,n,samples_file);
 if(audio_format==0x8120) {
  const float *s=(const float *)buf;
  for(int i=0;i<n/4;i++) { double v=fabs(s[i]); audio_energy+=v*v; if(v>audio_peak)audio_peak=v; if(v>1)clipped++; samples_count++; }
 }
}
static void audio_trampoline(void *ud,uint8_t *buf,int n) {(void)ud; audio_cb(audio_ud,buf,n);save_audio(buf,n);}
int SDL_OpenAudio(AudioSpec *wanted,AudioSpec *got) {
 int (*real)(AudioSpec *,AudioSpec *)=dlsym(RTLD_NEXT,"SDL_OpenAudio");
 AudioSpec copy=*wanted; audio_rate=copy.freq;audio_channels=copy.channels;audio_format=copy.format;
 if(copy.callback){audio_cb=copy.callback;audio_ud=copy.userdata;copy.callback=audio_trampoline;}
 return real(&copy,got);
}
unsigned SDL_OpenAudioDevice(const char *device,int capture,const AudioSpec *wanted,AudioSpec *got,int flags) {
 unsigned (*real)(const char *,int,const AudioSpec *,AudioSpec *,int)=dlsym(RTLD_NEXT,"SDL_OpenAudioDevice");
 AudioSpec copy=*wanted;audio_rate=copy.freq;audio_channels=copy.channels;audio_format=copy.format;
 if(copy.callback){audio_cb=copy.callback;audio_ud=copy.userdata;copy.callback=audio_trampoline;}
 return real(device,capture,&copy,got,flags);
}
int SDL_QueueAudio(unsigned device,const void *data,unsigned len) {
 int (*real)(unsigned,const void *,unsigned)=dlsym(RTLD_NEXT,"SDL_QueueAudio");save_audio(data,len);return real(device,data,len);
}
static void capture_frame(double t) {
 void (*getint)(unsigned,int*)=dlsym(RTLD_NEXT,"glGetIntegerv");
 void (*readpx)(int,int,int,int,unsigned,unsigned,void*)=dlsym(RTLD_NEXT,"glReadPixels");
 void (*store)(unsigned,int)=dlsym(RTLD_NEXT,"glPixelStorei");
 if(!getint||!readpx)return;
 int vp[4];getint(0xba2,vp); if(vp[2]<1||vp[3]<1)return;
 size_t stride=vp[2]*3;uint8_t *pixels=malloc(stride*vp[3]);if(!pixels)return;
 if(store)store(0xd05,1);readpx(vp[0],vp[1],vp[2],vp[3],0x1907,0x1401,pixels);
 char path[4200];snprintf(path,sizeof path,"%s/frame-%02d.ppm",folder,next_shot);
 FILE *f=fopen(path,"wb");if(f){fprintf(f,"P6\n%d %d\n255\n",vp[2],vp[3]);for(int y=vp[3]-1;y>=0;y--)fwrite(pixels+y*stride,1,stride,f);fclose(f);}
 double mean=0,energy=0;for(size_t i=0;i<stride*vp[3];i++){double v=pixels[i]/255.;mean+=v;energy+=v*v;}
 fprintf(stderr,"AUDIT frame=%d time=%.3f size=%dx%d mean=%.5f rms=%.5f\n",next_shot,t,vp[2],vp[3],mean/(stride*vp[3]),sqrt(energy/(stride*vp[3])));
 free(pixels);
}
void SDL_GL_SwapWindow(void *win) {
 void (*real)(void *)=dlsym(RTLD_NEXT,"SDL_GL_SwapWindow");
 double stamp=now(); if(!first){first=stamp; const unsigned char *(*getstr)(unsigned)=dlsym(RTLD_NEXT,"glGetString"); if(getstr)fprintf(stderr,"AUDIT renderer=%s version=%s\n",getstr(0x1F01),getstr(0x1F02));} if(last){double gap=stamp-last;sum_gap+=gap;if(gap>max_gap)max_gap=gap;}last=stamp;frames++;
 init();double time=stamp-first;
 static const double shots[]={3,12,23,34,44,53,57,59};
 if(folder[0]&&next_shot<8&&time>=shots[next_shot]){capture_frame(time);next_shot++;}
 real(win);
}
int SDL_PollEvent(void *event) {
 int (*real)(void *)=dlsym(RTLD_NEXT,"SDL_PollEvent");
 const char *v=getenv("MORPH_AUDIT_ESCAPE");
 if(v&&first&&!escape_sent&&now()-first>=atof(v)&&event){
  memset(event,0,56); *(uint32_t*)event=0x300;*((uint8_t*)event+12)=1;
  *(int32_t*)((uint8_t*)event+16)=41;*(int32_t*)((uint8_t*)event+20)=27;escape_sent=1;return 1;
 }
 return real(event);
}
static void finish(void) {
 if(finished||!first)return;finished=1;
 if(samples_file){fclose(samples_file);samples_file=0;}
 init();if(!folder[0])return;char path[4200];snprintf(path,sizeof path,"%s/metrics.json",folder);FILE *f=fopen(path,"w");
 if(f){fprintf(f,"{\n  \"frames\": %lu,\n  \"render_seconds\": %.6f,\n  \"mean_fps\": %.3f,\n  \"max_frame_gap\": %.6f,\n  \"audio_rate\": %d,\n  \"audio_channels\": %d,\n  \"audio_format\": %d,\n  \"audio_samples\": %lu,\n  \"audio_peak\": %.8f,\n  \"audio_rms\": %.8f,\n  \"audio_clipped\": %lu,\n  \"escape_injected\": %s\n}\n",frames,last-first,(frames-1)/sum_gap,max_gap,audio_rate,audio_channels,audio_format,samples_count,audio_peak,samples_count?sqrt(audio_energy/samples_count):0,clipped,escape_sent?"true":"false");fclose(f);}
}
void SDL_Quit(void) {void (*real)(void)=dlsym(RTLD_NEXT,"SDL_Quit");real();finish();}
__attribute__((destructor))static void audit_finish(void){finish();}
