// LD_PRELOAD verification hook for the demos (development tool, not part of the deliverables).
//   HOOK_OUT=dir        output directory (default .)
//   HOOK_SHOTS=t1,t2..  save a PPM of the frame presented first at/after these demo times (seconds)
//   HOOK_DIV=n          downscale saved shots by the integer factor n (box filter)
//   HOOK_EVERY=dt       save a shot every dt seconds of demo time (in addition to HOOK_SHOTS), e.g. 0.5
//   HOOK_NAN=k          every k-th frame, read back the HDR target (bound at glGenerateMipmap) and report NaN/Inf pixels
//   HOOK_AUDIO=1        write everything passed to SDL_QueueAudio to <dir>/audio.raw
//   HOOK_ESC=t          inject an Escape key press at demo time t (seconds)
//   HOOK_SPEED=k        run the demo clock k times faster (SDL_GetTicks is scaled)
//   HOOK_WINDOW=WxH     open an ordinary window of that size instead of the fullscreen one (controlled aspect ratio for tests)
//   HOOK_SPEEDS=t:k,...   piecewise clock speed by demo time, e.g. 0:8,44:1,49:8 (fast until 44 s, real time until 49 s, fast again)
//   HOOK_DUMP=addr:len:file  at the first SDL_Init call write len bytes of process memory at addr to file (the unpacked payload)
//   HOOK_RESIZE=t:WxH   call SDL_SetWindowSize(W,H) once when the demo clock passes t (tests size changes while running; needs HOOK_WINDOW)
//   HOOK_LOG=1          write <dir>/frames.log (one line per presented frame: real ms since start, demo ms)
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

static double now(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return ts.tv_sec+ts.tv_nsec*1e-9; }
static double t_start, t_first=-1;
static const char *outdir="."; static double speed=1.0;
static double shots[64]; static int nshots, shot_done[64];
static double esc_at=-1; static int esc_sent; static int shot_div=1; static double every=0, next_every=0;
static FILE *logf; static FILE *audf;
static int frames;
static uint32_t (*real_ticks)(void);
static int clock_started; static double demo_ms_base=-1; static uint32_t first_ticks; static double sp_t[16], sp_k[16]; static int nsp; static double acc_ms=0; static uint32_t last_real=0;

__attribute__((constructor)) static void init(void){
  t_start=now();
  const char *s;
  if((s=getenv("HOOK_OUT"))) outdir=s;
  if((s=getenv("HOOK_SPEED"))) speed=atof(s);
  if((s=getenv("HOOK_SPEEDS"))){ char *d=strdup(s),*t=strtok(d,","); while(t&&nsp<16){ sscanf(t,"%lf:%lf",&sp_t[nsp],&sp_k[nsp]); nsp++; t=strtok(0,","); } }
  if((s=getenv("HOOK_SHOTS"))){ char *d=strdup(s),*t=strtok(d,","); while(t&&nshots<64){ shots[nshots++]=atof(t); t=strtok(0,","); } }
  if((s=getenv("HOOK_ESC"))) esc_at=atof(s);
  if((s=getenv("HOOK_DIV"))) shot_div=atoi(s)>0?atoi(s):1;
  if((s=getenv("HOOK_EVERY"))) every=atof(s);
  char fn[600];
  if(getenv("HOOK_LOG")){ snprintf(fn,sizeof fn,"%s/frames.log",outdir); logf=fopen(fn,"w"); if(logf) setvbuf(logf,0,_IOLBF,0); }
  if(getenv("HOOK_AUDIO")){ snprintf(fn,sizeof fn,"%s/audio.raw",outdir); audf=fopen(fn,"wb"); }
}

// scaled clock: the first call defines the origin, later calls are stretched
int SDL_Init(uint32_t flags){
  static int (*f)(uint32_t); if(!f) f=dlsym(RTLD_NEXT,"SDL_Init");
  const char *d=getenv("HOOK_DUMP");
  if(d){ unsigned long a,l; char fn[600]; if(sscanf(d,"%lx:%lu:%599s",&a,&l,fn)==3){ FILE *o=fopen(fn,"wb"); if(o){ fwrite((void*)a,1,l,o); fclose(o); fprintf(stderr,"[hook] dumped %lu bytes at 0x%lx to %s\n",l,a,fn); } } }
  return f(flags);
}
uint32_t SDL_GetTicks(void){
  if(!real_ticks) real_ticks=dlsym(RTLD_NEXT,"SDL_GetTicks");
  uint32_t t=real_ticks();
  if(!clock_started) return t;                       // before the audio starts the clock is untouched
  if(nsp){
    double dt=(double)(t-last_real); last_real=t;
    double now_demo=acc_ms/1000.0; double k=speed;
    for(int i=0;i<nsp;i++) if(now_demo>=sp_t[i]) k=sp_k[i];
    acc_ms+=dt*k;
    return first_ticks+(uint32_t)acc_ms;
  }
  if(speed==1.0) return t;
  return first_ticks+(uint32_t)((t-first_ticks)*speed);
}
static double demo_time(void){ uint32_t t=SDL_GetTicks(); return (t-first_ticks)/1000.0; }

int SDL_QueueAudio(uint32_t dev,const void*data,uint32_t len){
  static int (*f)(uint32_t,const void*,uint32_t); if(!f) f=dlsym(RTLD_NEXT,"SDL_QueueAudio");
  fprintf(stderr,"[hook %.3fs] SDL_QueueAudio dev=%u len=%u\n",now()-t_start,dev,len);
  if(audf){ fwrite(data,1,len,audf); fflush(audf); }
  return f(dev,data,len);
}
void SDL_PauseAudioDevice(uint32_t dev,int on){
  static void (*f)(uint32_t,int); if(!f) f=dlsym(RTLD_NEXT,"SDL_PauseAudioDevice");
  fprintf(stderr,"[hook %.3fs] SDL_PauseAudioDevice dev=%u on=%d\n",now()-t_start,dev,on);
  if(!on && !clock_started){ if(!real_ticks) real_ticks=dlsym(RTLD_NEXT,"SDL_GetTicks"); first_ticks=real_ticks(); last_real=first_ticks; acc_ms=0; demo_ms_base=first_ticks; clock_started=1; }
  f(dev,on);
}
int SDL_PollEvent(void *ev){
  static int (*f)(void*); if(!f) f=dlsym(RTLD_NEXT,"SDL_PollEvent");
  if(esc_at>=0 && !esc_sent && frames>0 && demo_time()>=esc_at){
    esc_sent=1; memset(ev,0,56); *(uint32_t*)ev=0x300; ((uint8_t*)ev)[12]=1; *(int32_t*)((uint8_t*)ev+16)=41; *(int32_t*)((uint8_t*)ev+20)=27;
    fprintf(stderr,"[hook %.3fs] injected Escape at demo time %.3f\n",now()-t_start,demo_time());
    return 1;
  }
  return f(ev);
}
void *SDL_CreateWindow(const char *title,int x,int y,int w,int h,uint32_t flags){
  static void *(*f)(const char*,int,int,int,int,uint32_t); if(!f) f=dlsym(RTLD_NEXT,"SDL_CreateWindow");
  const char *e=getenv("HOOK_WINDOW"); int W,H;
  if(e && sscanf(e,"%dx%d",&W,&H)==2){ flags=(flags&~0x1001u)|0x4u; w=W; h=H; fprintf(stderr,"[hook] window forced to %dx%d\n",W,H); }
  return f(title,x,y,w,h,flags);
}
unsigned glCreateShaderProgramv(unsigned type,int count,const char*const*strs){
  static unsigned (*f)(unsigned,int,const char*const*); if(!f) f=dlsym(RTLD_NEXT,"glCreateShaderProgramv");
  static void (*gpi)(unsigned,unsigned,int*); static void (*gil)(unsigned,int,int*,char*);
  if(!gpi){ gpi=dlsym(RTLD_DEFAULT,"glGetProgramiv"); gil=dlsym(RTLD_DEFAULT,"glGetProgramInfoLog"); }
  unsigned p=f(type,count,strs); int ok=0,ll=0; gpi(p,0x8B82,&ok); gpi(p,0x8B84,&ll);
  fprintf(stderr,"[hook %.3fs] glCreateShaderProgramv type=0x%x -> program %u link=%d source=%zu bytes\n",now()-t_start,type,p,ok,strlen(strs[0]));
  if(ll>1){ char *log=malloc(ll+1); gil(p,ll,0,log); fprintf(stderr,"[hook] shader log: %s\n",log); free(log); }
  return p;
}
void glCompileShader(unsigned sh){
  static void (*f)(unsigned); if(!f) f=dlsym(RTLD_NEXT,"glCompileShader");
  static void (*gsi)(unsigned,unsigned,int*); static void (*gil)(unsigned,int,int*,char*);
  if(!gsi){ gsi=dlsym(RTLD_DEFAULT,"glGetShaderiv"); gil=dlsym(RTLD_DEFAULT,"glGetShaderInfoLog"); }
  f(sh); int ok=0,ll=0; gsi(sh,0x8B81,&ok); gsi(sh,0x8B84,&ll);
  fprintf(stderr,"[hook %.3fs] glCompileShader %u -> %s\n",now()-t_start,sh,ok?"ok":"FAILED");
  if(ll>1){ char *log=malloc(ll+1); gil(sh,ll,0,log); fprintf(stderr,"[hook] compile log: %s\n",log); free(log); }
}
void glLinkProgram(unsigned p){
  static void (*f)(unsigned); if(!f) f=dlsym(RTLD_NEXT,"glLinkProgram");
  static void (*gpi)(unsigned,unsigned,int*); static void (*gil)(unsigned,int,int*,char*);
  if(!gpi){ gpi=dlsym(RTLD_DEFAULT,"glGetProgramiv"); gil=dlsym(RTLD_DEFAULT,"glGetProgramInfoLog"); }
  f(p); int ok=0,ll=0; gpi(p,0x8B82,&ok); gpi(p,0x8B84,&ll);
  fprintf(stderr,"[hook %.3fs] glLinkProgram %u -> %s\n",now()-t_start,p,ok?"ok":"FAILED");
  if(ll>1){ char *log=malloc(ll+1); gil(p,ll,0,log); fprintf(stderr,"[hook] link log: %s\n",log); free(log); }
}
void glGenerateMipmap(unsigned target){
  static void (*f)(unsigned); if(!f) f=dlsym(RTLD_NEXT,"glGenerateMipmap");
  static int nanevery=-1, count=0, reported=0; static double worst=0;
  if(nanevery<0){ const char *e=getenv("HOOK_NAN"); nanevery=e?atoi(e):0; }
  if(nanevery>0 && (count++%nanevery)==0){
    static void (*gi)(unsigned,int*); static void (*gtl)(unsigned,int,unsigned,int*); static void (*gti)(unsigned,int,unsigned,unsigned,void*); static unsigned (*ge)(void);
    if(!gi){ gi=dlsym(RTLD_DEFAULT,"glGetIntegerv"); gtl=dlsym(RTLD_DEFAULT,"glGetTexLevelParameteriv"); gti=dlsym(RTLD_DEFAULT,"glGetTexImage"); }
    int w=0,h=0; gtl(0x0DE1,0,0x1000,&w); gtl(0x0DE1,0,0x1001,&h);
    if(w>0&&h>0){
      float *px=malloc((size_t)w*h*16); gti(0x0DE1,0,0x1908,0x1406,px);
      long bad=0; long first=-1; float mx=0;
      for(long i=0;i<(long)w*h*4;i++){ float v=px[i]; if(v!=v || v>1e30f || v<-1e30f){ bad++; if(first<0) first=i/4; } else if(i%4<3 && v>mx) mx=v; }
      if(mx>worst) worst=mx;
      if(bad && reported<20){ reported++; fprintf(stderr,"[hook NAN] demo t=%.2f: %ld bad channel values, first pixel (%ld,%ld) of %dx%d\n",demo_time(),bad,first%w,first/w,w,h); }
      else if(!bad && (count%(nanevery*60))==1) fprintf(stderr,"[hook NAN] demo t=%.2f: clean (max HDR value so far %.1f)\n",demo_time(),worst);
      free(px);
    }
  }
  f(target);
}
void glDrawArrays(unsigned mode,int first,int count){
  static void (*f)(unsigned,int,int); if(!f) f=dlsym(RTLD_NEXT,"glDrawArrays");
  static unsigned (*ge)(void); static unsigned (*cfs)(unsigned); if(!ge){ ge=dlsym(RTLD_DEFAULT,"glGetError"); cfs=dlsym(RTLD_DEFAULT,"glCheckFramebufferStatus"); }
  static int n; f(mode,first,count);
  if(n++<3){ unsigned e=ge(); unsigned fs=cfs(0x8D40); fprintf(stderr,"[hook] glDrawArrays(mode=%u,count=%d) glError=0x%x fbstatus=0x%x\n",mode,count,e,fs); }
}
void SDL_GL_SwapWindow(void *w){
  static void (*f)(void*); if(!f) f=dlsym(RTLD_NEXT,"SDL_GL_SwapWindow");
  static void (*rp)(int,int,int,int,unsigned,unsigned,void*); static void (*gi)(unsigned,int*); static void (*fin)(void);
  if(!rp){ rp=dlsym(RTLD_DEFAULT,"glReadPixels"); gi=dlsym(RTLD_DEFAULT,"glGetIntegerv"); fin=dlsym(RTLD_DEFAULT,"glFinish"); }
  double r=now()-t_start; if(t_first<0){ t_first=r; fprintf(stderr,"[hook %.3fs] first frame\n",r); }
  double dt=demo_time();
  { static int done; static double rt=-1; static int rw,rh; static int init;
    if(!init){ init=1; const char *e=getenv("HOOK_RESIZE"); if(e) sscanf(e,"%lf:%dx%d",&rt,&rw,&rh); }
    if(rt>=0 && !done && dt>=rt){ done=1; void (*sws)(void*,int,int)=dlsym(RTLD_DEFAULT,"SDL_SetWindowSize"); if(sws){ sws(w,rw,rh); fprintf(stderr,"[hook] window resized to %dx%d at demo time %.2f\n",rw,rh,dt); } } }
  if(logf) fprintf(logf,"%.4f %.4f\n",r,dt);
  int want=-1; double want_t=0;
  for(int i=0;i<nshots;i++) if(!shot_done[i] && dt>=shots[i]){ shot_done[i]=1; want=i; want_t=shots[i]; break; }
  if(want<0 && every>0 && dt>=next_every){ want=1000; want_t=dt; next_every=dt+every; }
  if(want>=0 && rp){
    int vp[4]; gi(0x0BA2,vp); int W=vp[2],H=vp[3]; unsigned char *px=malloc((size_t)W*H*4);
    fin(); rp(0,0,W,H,0x1908,0x1401,px);
    int D=shot_div; int w2=W/D,h2=H/D;
    char fn[700]; snprintf(fn,sizeof fn,"%s/shot_%06.2f.ppm",outdir,want_t); FILE *o=fopen(fn,"wb"); fprintf(o,"P6\n%d %d\n255\n",w2,h2);
    for(int y=h2-1;y>=0;y--) for(int x=0;x<w2;x++){
      int acc[3]={0,0,0};
      for(int dy=0;dy<D;dy++) for(int dx=0;dx<D;dx++){ unsigned char *q=px+((size_t)(y*D+dy)*W+(x*D+dx))*4; acc[0]+=q[0]; acc[1]+=q[1]; acc[2]+=q[2]; }
      unsigned char c[3]={acc[0]/(D*D),acc[1]/(D*D),acc[2]/(D*D)}; fwrite(c,1,3,o);
    }
    fclose(o); free(px); fprintf(stderr,"[hook %.3fs] shot %s (%dx%d) at demo time %.3f\n",r,fn,w2,h2,dt);
  }
  frames++;
  f(w);
}
