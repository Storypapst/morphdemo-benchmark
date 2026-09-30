// Development harness (not part of the deliverables).
//   gltool render  <frag.glsl> <outprefix> <W> <H> <t0> [t1 ...]   render fragment shader frames to PPM
//   gltool bench   <frag.glsl> <W> <H> <t0> [t1 ...]              GPU time per frame at the given times
//   gltool audio   <comp.glsl> <seconds> <rate> <out.f32>         run a compute shader that fills vec2 o[] and dump floats
// Shader interface (same as the demos): fragment  layout(location=0) uniform vec3 u; (u.x=time, u.yz=resolution)
//                                       compute   layout(std430,binding=0) buffer B{vec2 o[];};  local_size_x=64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

int SDL_Init(uint32_t);
void *SDL_CreateWindow(const char*, int, int, int, int, uint32_t);
void *SDL_GL_CreateContext(void*);
void *SDL_GL_GetProcAddress(const char*);
void SDL_GL_SwapWindow(void*);
const char *SDL_GetError(void);
void SDL_Quit(void);

typedef unsigned int GLuint; typedef int GLint; typedef unsigned int GLenum; typedef int GLsizei;
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPUTE_SHADER 0x91B9
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_FRAMEBUFFER 0x8D40
#define GL_TEXTURE_2D 0x0DE1
#define GL_RGBA8 0x8058
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#define GL_DYNAMIC_READ 0x88E9
#define GL_READ_ONLY 0x88B8
#define GL_BUFFER_UPDATE_BARRIER_BIT 0x200
#define GL_SHADER_STORAGE_BARRIER_BIT 0x2000

#define GLFUNCS \
 F(GLuint,glCreateShaderProgramv,(GLenum,GLsizei,const char*const*)) \
 F(void,glGetProgramiv,(GLuint,GLenum,GLint*)) \
 F(void,glGetProgramInfoLog,(GLuint,GLsizei,GLsizei*,char*)) \
 F(void,glUseProgram,(GLuint)) \
 F(void,glUniform3i,(GLint,GLint,GLint,GLint)) \
 F(void,glUniform1i,(GLint,GLint)) \
 F(void,glRects,(short,short,short,short)) \
 F(void,glViewport,(GLint,GLint,GLsizei,GLsizei)) \
 F(void,glReadPixels,(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*)) \
 F(void,glFinish,(void)) \
 F(void,glGenFramebuffers,(GLsizei,GLuint*)) \
 F(void,glBindFramebuffer,(GLenum,GLuint)) \
 F(void,glFramebufferTexture2D,(GLenum,GLenum,GLenum,GLuint,GLint)) \
 F(void,glGenTextures,(GLsizei,GLuint*)) \
 F(void,glBindTexture,(GLenum,GLuint)) \
 F(void,glTexImage2D,(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*)) \
 F(void,glGenBuffers,(GLsizei,GLuint*)) \
 F(void,glBindBuffer,(GLenum,GLuint)) \
 F(void,glBufferData,(GLenum,long,const void*,GLenum)) \
 F(void,glBindBufferBase,(GLenum,GLuint,GLuint)) \
 F(void,glDispatchCompute,(GLuint,GLuint,GLuint)) \
 F(void,glMemoryBarrier,(GLuint)) \
 F(void*,glMapBuffer,(GLenum,GLenum)) \
 F(unsigned char,glUnmapBuffer,(GLenum)) \
 F(const unsigned char*,glGetString,(GLenum))
#define F(r,n,a) static r (*n)a;
GLFUNCS
#undef F

static double now(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return ts.tv_sec+ts.tv_nsec*1e-9; }

static char *slurp(const char *fn, long *len){
  FILE *f=fopen(fn,"rb"); if(!f){ fprintf(stderr,"cannot open %s\n",fn); exit(2);} fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
  char *b=malloc(n+1); if(fread(b,1,n,f)!=(size_t)n){ exit(2);} b[n]=0; fclose(f); if(len)*len=n; return b;
}
static GLuint build(GLenum type, const char *src, const char *name){
  GLuint p=glCreateShaderProgramv(type,1,&src); GLint ok=0; glGetProgramiv(p,GL_LINK_STATUS,&ok);
  GLint ll=0; glGetProgramiv(p,GL_INFO_LOG_LENGTH,&ll);
  if(ll>1){ char *log=malloc(ll+1); glGetProgramInfoLog(p,ll,0,log); fprintf(stderr,"[%s] %s\n",name,log); free(log);} 
  if(!ok){ fprintf(stderr,"[%s] shader build FAILED\n",name); exit(3);} return p;
}
static void init(void){
  if(SDL_Init(0x20)){ fprintf(stderr,"SDL_Init: %s\n",SDL_GetError()); exit(1);} 
  void *w=SDL_CreateWindow("gltool",0,0,64,64,0x2|0x8); // OPENGL|HIDDEN
  if(!w){ fprintf(stderr,"window: %s\n",SDL_GetError()); exit(1);} 
  if(!SDL_GL_CreateContext(w)){ fprintf(stderr,"ctx: %s\n",SDL_GetError()); exit(1);} 
#define F(r,n,a) n=(r(*)a)SDL_GL_GetProcAddress(#n); if(!n){ fprintf(stderr,"missing %s\n",#n); exit(1);} 
  GLFUNCS
#undef F
}

static void fbo_setup(int W,int H){
  GLuint tex,fb; glGenTextures(1,&tex); glBindTexture(GL_TEXTURE_2D,tex);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,W,H,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
  glGenFramebuffers(1,&fb); glBindFramebuffer(GL_FRAMEBUFFER,fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);
  glViewport(0,0,W,H);
}

int main(int argc,char**argv){
  if(argc<2){ fprintf(stderr,"usage: see source\n"); return 1; }
  init();
  if(!strcmp(argv[1],"render") && argc>=7){
    char *src=slurp(argv[2],0); const char *pre=argv[3]; int W=atoi(argv[4]),H=atoi(argv[5]);
    GLuint p=build(GL_FRAGMENT_SHADER,src,argv[2]); fbo_setup(W,H); glUseProgram(p);
    unsigned char *px=malloc((size_t)W*H*4);
    for(int i=6;i<argc;i++){
      float t=atof(argv[i]); glUniform3i(0,(int)(t*1000+.5),W,H); glRects(-1,-1,1,1); glFinish();
      glReadPixels(0,0,W,H,GL_RGBA,GL_UNSIGNED_BYTE,px);
      char fn[512]; snprintf(fn,sizeof fn,"%s_%03d.ppm",pre,i-6); FILE *f=fopen(fn,"wb"); fprintf(f,"P6\n%d %d\n255\n",W,H);
      for(int y=H-1;y>=0;y--) for(int x=0;x<W;x++){ fwrite(px+((size_t)y*W+x)*4,1,3,f);} fclose(f);
    }
    return 0;
  }
  if(!strcmp(argv[1],"bench") && argc>=6){
    char *src=slurp(argv[2],0); int W=atoi(argv[3]),H=atoi(argv[4]);
    GLuint p=build(GL_FRAGMENT_SHADER,src,argv[2]); fbo_setup(W,H); glUseProgram(p);
    for(int i=5;i<argc;i++){
      float t=atof(argv[i]); for(int k=0;k<3;k++){ glUniform3i(0,(int)((t+k*0.01f)*1000),W,H); glRects(-1,-1,1,1);} glFinish();
      double t0=now(); int n=20; for(int k=0;k<n;k++){ glUniform3i(0,(int)((t+k*0.001f)*1000),W,H); glRects(-1,-1,1,1);} glFinish();
      printf("t=%6.2f  %.3f ms/frame @%dx%d\n",t,(now()-t0)*1000/n,W,H);
    }
    return 0;
  }
  if(!strcmp(argv[1],"audio") && argc>=6){
    char *src=slurp(argv[2],0); double sec=atof(argv[3]); int rate=atoi(argv[4]);
    long n=(long)(sec*rate); n=(n+63)&~63L; double t0=now();
    GLuint p=build(GL_COMPUTE_SHADER,src,argv[2]); double t1=now();
    int lean=getenv("LEAN")!=0; GLuint buf=1;
    if(lean){ glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,1); glBufferData(GL_SHADER_STORAGE_BUFFER,n*(getenv("STRIDE16")?16:8),0,GL_DYNAMIC_READ); }
    else { glGenBuffers(1,&buf); glBindBuffer(GL_SHADER_STORAGE_BUFFER,buf);
    glBufferData(GL_SHADER_STORAGE_BUFFER,n*(getenv("STRIDE16")?16:8),0,GL_DYNAMIC_READ); glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buf); }
    glUseProgram(p); glDispatchCompute(n/64,1,1); if(!lean) { glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT|GL_SHADER_STORAGE_BARRIER_BIT); glFinish(); } double t2=now();
    float *m=glMapBuffer(GL_SHADER_STORAGE_BUFFER,GL_READ_ONLY); FILE *f=fopen(argv[5],"wb"); fwrite(m,getenv("STRIDE16")?16:8,n,f); fclose(f); glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    fprintf(stderr,"audio: %ld frames, compile %.3fs, compute %.3fs\n",n,t1-t0,t2-t1);
    return 0;
  }
  if(!strcmp(argv[1],"audio2") && argc>=7){
    // two passes: aud.comp (writes vec4 dry+send to binding 0), rev.comp (reads binding 0, writes vec2 to binding 1, uniform base at location 1)
    char *sa=slurp(argv[2],0),*sr=slurp(argv[3],0); double sec=atof(argv[4]); int rate=atoi(argv[5]);
    long n=(long)(sec*rate); n=(n+63)&~63L; double t0=now();
    GLuint pa=build(GL_COMPUTE_SHADER,sa,argv[2]),pr=build(GL_COMPUTE_SHADER,sr,argv[3]); double t1=now();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,1); glBufferData(GL_SHADER_STORAGE_BUFFER,n*16,0,GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,2); glBufferData(GL_SHADER_STORAGE_BUFFER,n*8,0,GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,1);
    glUseProgram(pa); glDispatchCompute(n/64,1,1); glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); glFinish(); double t2=now();
    glUseProgram(pr); long chunk=1<<17;
    for(long b=0;b<n;b+=chunk){ long c=n-b<chunk?n-b:chunk; glUniform1i(1,(GLint)b); glDispatchCompute(c/64,1,1); glFinish(); }
    double t3=now();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,2);
    float *m=glMapBuffer(GL_SHADER_STORAGE_BUFFER,GL_READ_ONLY); FILE *f=fopen(argv[6],"wb"); fwrite(m,8,n,f); fclose(f); glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    fprintf(stderr,"audio2: %ld frames, compile %.3fs, pass1 %.3fs, reverb %.3fs\n",n,t1-t0,t2-t1,t3-t2);
    return 0;
  }
  fprintf(stderr,"bad arguments\n"); return 1;
}
