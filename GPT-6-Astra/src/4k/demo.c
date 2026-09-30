/* PHASE LOCK -- all pictures and sound are calculated, without assets. */
typedef unsigned int U;
typedef struct { int freq; unsigned short format; unsigned char channels,silence; unsigned short samples,padding; U size; void (*callback)(void*,unsigned char*,int); void *userdata; } Audio;
extern int SDL_Init(U), SDL_PollEvent(void*), SDL_OpenAudio(Audio*,Audio*);
extern void *SDL_CreateWindow(const char*,int,int,int,int,U), *SDL_GL_CreateContext(void*);
extern void SDL_GL_SwapWindow(void*), SDL_GL_GetDrawableSize(void*,int*,int*), SDL_PauseAudio(int), SDL_Quit(void), SDL_Delay(U);
extern int SDL_GL_SetSwapInterval(int),SDL_ShowCursor(int);
extern U SDL_GetTicks(void);
extern U glCreateShaderProgramv(U,int,const char**);
extern void glUseProgram(U),glUniform3f(int,float,float,float),glRectf(float,float,float,float),glViewport(int,int,int,int);
extern int glGetUniformLocation(U,const char*);
extern float sinf(float),exp2f(float),expf(float);
extern void _exit(int);
static const char *shader=
#include "shader.h"
;
static unsigned samples;
static float clip(float x) {return x<0?0:x>1?1:x;}
static void music(void *unused,unsigned char *data,int length) {
    float *out=(float*)data;
    for(int i=0;i<length/8;i++,samples++) {
        float t=samples/48000.f,b=t*2,f=b-(int)b,q=t*4-(int)(t*4);
        int roots[]={0,-5,3,-2},notes[]={0,7,12,15,19,15,12,7};
        float root=55*exp2f(roots[t<40?(int)(t/8)%4:0]/12.f);
        float arp=root*4*exp2f(notes[(int)(t*4)%8]/12.f);
        float grow=clip((t-12)/16),fade=clip(t/3)*clip((60-t)/5);
        float bell=sinf(arp*6.283185f*t+.6f*sinf(arp*12.56637f*t))*expf(-5*q)*grow;
        float kick=sinf(70*f-9*expf(-25*f))*expf(-12*f)*grow;
        float hat=sinf(t*110013)*sinf(t*93117)*expf(-36*q)*grow;
        float pad=sinf(root*6.283185f*t)+.4f*sinf(root*9.424778f*t)+.25f*sinf(root*15.70796f*t);
        float chord=sinf(t*.392699f);
        float y=fade*(.10f*pad*chord*chord+.14f*bell+.26f*kick+.025f*hat);
        out[i*2]=y*(.9f+.1f*sinf(t*.7f));
        out[i*2+1]=y*(.9f-.1f*sinf(t*.7f));
    }
}
static Audio audio={48000,0x8120,2,0,1024,0,0,music,0};
__attribute__((used)) void demo(void) {
    if(SDL_Init(0x31)) _exit(1);
    void *window=SDL_CreateWindow("MORPH / PHASE LOCK",0x2fff0000,0x2fff0000,1920,1080,0x1003);
    if(!window || !SDL_GL_CreateContext(window)) _exit(2);
    SDL_ShowCursor(0); SDL_GL_SetSwapInterval(1);
    U program=glCreateShaderProgramv(0x8b30,1,&shader);
    glUseProgram(program);
    int uniform=glGetUniformLocation(program,"u"),w,h;
    SDL_GL_GetDrawableSize(window,&w,&h);glViewport(0,0,w,h);
    if(SDL_OpenAudio(&audio,0)) {SDL_Quit();_exit(3);}
    U start=SDL_GetTicks();SDL_PauseAudio(0);
    char event[64];
    for(;;) {
        float t=(SDL_GetTicks()-start)*.001f;
        if(t>=60) break;
        while(SDL_PollEvent(event)) if(*(U*)event==0x100 || (*(U*)event==0x300 && *(int*)(event+20)==27)) goto done;
        glUniform3f(uniform,t,w,h);
        glRectf(-1,-1,1,1);SDL_GL_SwapWindow(window);
    }
done:SDL_Quit();_exit(0);
}
__attribute__((naked,noreturn)) void _start(void) {__asm__("andq $-16,%rsp;call demo");}
