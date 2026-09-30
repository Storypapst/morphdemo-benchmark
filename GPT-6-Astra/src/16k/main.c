/* CRYSTAL ASSEMBLY / a one-minute procedural audiovisual miniature. */
#include <math.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {int freq;uint16_t format;uint8_t channels,silence;uint16_t samples,padding;uint32_t size;void(*callback)(void*,uint8_t*,int);void *userdata;} AudioSpec;
extern int SDL_Init(uint32_t);extern const char *SDL_GetError(void);extern void SDL_Quit(void);extern void *SDL_CreateWindow(const char*,int,int,int,int,uint32_t);extern void SDL_DestroyWindow(void*);extern void *SDL_GL_CreateContext(void*);extern void SDL_GL_DeleteContext(void*);extern int SDL_GL_SetAttribute(int,int);extern int SDL_GL_SetSwapInterval(int);extern void SDL_GL_SwapWindow(void*);extern void SDL_GL_GetDrawableSize(void*,int*,int*);extern int SDL_PollEvent(void*);extern uint32_t SDL_GetTicks(void);extern void SDL_Delay(uint32_t);extern int SDL_ShowCursor(int);extern uint32_t SDL_OpenAudioDevice(const char*,int,const AudioSpec*,AudioSpec*,int);extern void SDL_PauseAudioDevice(uint32_t,int);extern void SDL_CloseAudioDevice(uint32_t);
extern uint32_t glCreateShader(uint32_t);extern void glShaderSource(uint32_t,int,const char*const*,const int*);extern void glCompileShader(uint32_t);extern void glGetShaderiv(uint32_t,uint32_t,int*);extern void glGetShaderInfoLog(uint32_t,int,int*,char*);extern uint32_t glCreateProgram(void);extern void glAttachShader(uint32_t,uint32_t);extern void glLinkProgram(uint32_t);extern void glGetProgramiv(uint32_t,uint32_t,int*);extern void glGetProgramInfoLog(uint32_t,int,int*,char*);extern void glUseProgram(uint32_t);extern int glGetUniformLocation(uint32_t,const char*);extern void glUniform1f(int,float);extern void glUniform2f(int,float,float);extern void glViewport(int,int,int,int);extern void glRects(short,short,short,short);
#include "shader.h"
static uint64_t sample;
static _Atomic uint64_t audio_clock;
static uint32_t rng=123456789;
static float delayL[32768],delayR[32768],lastnoise;
static float cap(float x){return fminf(fmaxf(x,0),1);}
static float envelope(float t,float a,float b){return cap(t/a)*cap((b-t)/a);}
static float hz(int n){return 440*exp2f((n-69)/12.f);}
static float sine(float x){return sinf(x*6.28318530718f);}
static void audio(void *u,uint8_t *stream,int bytes){
 (void)u;float *out=(float*)stream;static const int roots[]={38,34,41,36};static const int scale[]={0,7,12,15,19,22,19,15,7,12,24,19,15,12,7,3};
 for(int i=0;i<bytes/8;i++,sample++){
  float t=sample/48000.f,beat=t*2,phase=beat-floorf(beat),fade=envelope(t,3,60),build=cap((t-8)/16),ending=cap((56-t)/5);
  int chord=((int)t/4)%4,root=roots[chord];float f=hz(root),ca=fmodf(t,4);
  float pad=0;
  for(int j=0;j<4;j++){float nf=hz(root+12+(j==0?0:j==1?7:j==2?(chord==0?15:16):(chord==1?23:22)));pad+=sine(t*nf+.15*sine(t*.13+j))*.032f;}
  pad*=cap(ca*5)*cap((4-ca)*5);
  float bass=(sine(t*f)*.1f+sine(t*f*2)*.023f)*(.5+.5*expf(-phase*4))*build*cap(phase*70)*cap((1-phase)*30);
  int step=(int)(t*4);float age=t-step*.25f;int tone=scale[(step+chord*3)%16];if(chord&&tone%12==3)tone++;if(chord==1&&tone%12==10)tone++;int note=root+24+tone;
  float arpf=hz(note),pluck=(sine(age*arpf)+.24f*sine(age*arpf*2.002f)+.08f*sine(age*arpf*3))*expf(-age*13)*cap(age*300)*.085f*build;
  rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;float noise=(rng/(float)UINT32_MAX)*2-1,high=noise-lastnoise;lastnoise=noise;
  float drumOn=cap((t-15)/3)*ending;
  float kick=sine(phase*25+1.8f*(1-expf(-phase*24)))*expf(-phase*13)*.25f*drumOn;
  float snare=((int)beat%4==2)?(high*.11f+sine(phase*90)*.045f)*expf(-phase*25)*drumOn:0;
  float hat=high*expf(-fmodf(beat, .5f)*65)*.028f*drumOn;
  float air=noise*.008f*(1-build*.75f);
  float sig=pad+bass+pluck+kick+snare+hat+air;
  int pos=(int)sample&32767,read=((int)sample-18000)&32767;
  float l=delayL[read],r=delayR[(read-997)&32767];
  delayL[pos]=pad*.5f+pluck*.85f+r*.47f;delayR[pos]=pad*.5f+pluck*.6f+l*.47f;
  out[i*2]=(sig+l*.6f)*fade*.85f;out[i*2+1]=(sig+r*.6f)*fade*.85f;
 }
 atomic_store_explicit(&audio_clock,((uint64_t)SDL_GetTicks()<<32)|(uint32_t)sample,memory_order_relaxed);
}
static uint32_t shader(uint32_t kind,const char *s){uint32_t x=glCreateShader(kind);glShaderSource(x,1,&s,0);glCompileShader(x);int ok;glGetShaderiv(x,0x8B81,&ok);if(!ok){char b[4096];glGetShaderInfoLog(x,sizeof b,0,b);fprintf(stderr,"Crystal Assembly shader: %s\n",b);exit(2);}return x;}
int main(void){
 if(SDL_Init(0x30)){fprintf(stderr,"SDL: %s\n",SDL_GetError());return 1;}
 SDL_GL_SetAttribute(5,1);SDL_GL_SetAttribute(17,3);SDL_GL_SetAttribute(18,0);
 uint32_t flags=2|4|0x1001;
#ifdef DEVELOPMENT
 if(getenv("MORPH_WINDOWED"))flags=2|4;
#endif
 void *w=SDL_CreateWindow("MORPH / CRYSTAL ASSEMBLY",0x2fff0000,0x2fff0000,1920,1080,flags);
 if(!w){fprintf(stderr,"Window: %s\n",SDL_GetError());SDL_Quit();return 1;}
 void *context=SDL_GL_CreateContext(w);if(!context){fprintf(stderr,"OpenGL: %s\n",SDL_GetError());SDL_DestroyWindow(w);SDL_Quit();return 1;}
 SDL_GL_SetSwapInterval(1);SDL_ShowCursor(0);
 uint32_t program=glCreateProgram();glAttachShader(program,shader(0x8B31,"#version 130\nvoid main(){gl_Position=gl_Vertex;}"));glAttachShader(program,shader(0x8B30,fragment));glLinkProgram(program);int ok;glGetProgramiv(program,0x8B82,&ok);if(!ok){char b[4096];glGetProgramInfoLog(program,sizeof b,0,b);fprintf(stderr,"Link: %s\n",b);return 2;}
 glUseProgram(program);int timeLoc=glGetUniformLocation(program,"T"),resLoc=glGetUniformLocation(program,"R");
 AudioSpec want={48000,0x8120,2,0,1024,0,0,audio,0};
 uint32_t device=SDL_OpenAudioDevice(0,0,&want,0,0);if(!device){fprintf(stderr,"Audio: %s\n",SDL_GetError());SDL_GL_DeleteContext(context);SDL_DestroyWindow(w);SDL_Quit();return 1;}
 float offset=0;
#ifdef DEVELOPMENT
 if(getenv("MORPH_TIME")){offset=atof(getenv("MORPH_TIME"));sample=(uint64_t)(offset*48000);}
#endif
 uint32_t start=SDL_GetTicks();SDL_PauseAudioDevice(device,0);int running=1;
 while(running){uint8_t event[64];while(SDL_PollEvent(event)){uint32_t type;memcpy(&type,event,4);int sym;memcpy(&sym,event+20,4);if(type==0x100||(type==0x300&&sym==27))running=0;}uint32_t now=SDL_GetTicks();uint64_t ac=atomic_load_explicit(&audio_clock,memory_order_relaxed);float t=ac?fmaxf(0,((uint32_t)ac-1024)/48000.f)+(now-(uint32_t)(ac>>32))/1000.f:offset;if(t>=60||now-start>=65000)break;
  int width,height;SDL_GL_GetDrawableSize(w,&width,&height);glViewport(0,0,width,height);glUniform2f(resLoc,width,height);glUniform1f(timeLoc,t);glRects(-1,-1,1,1);SDL_GL_SwapWindow(w);
 }
 SDL_CloseAudioDevice(device);SDL_ShowCursor(1);SDL_GL_DeleteContext(context);SDL_DestroyWindow(w);SDL_Quit();return 0;
}
