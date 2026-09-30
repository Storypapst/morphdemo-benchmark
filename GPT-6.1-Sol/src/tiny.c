#include "platform.h"
#include <math.h>
#include <stdatomic.h>
#include "shader.h"
static _Atomic unsigned cursor;
static unsigned random_state=1;
static void audio(void *unused,uint8_t *bytes,int count){
    (void)unused;
    float *out=(float *)bytes;
    static const float notes[]={146.8324f,220.f,293.6648f,349.2282f,440.f,293.6648f,174.6141f,220.f};
    unsigned s=atomic_load_explicit(&cursor,memory_order_relaxed);
    for(int i=0;i<count/8;i++,s++){
        float t=s/48000.f,a=(s%12000)/48000.f,b=(s%24000)/48000.f;
        float order=fminf(1,t/24),ending=fmaxf(0,fminf(1,(64-t)/5));
        float n=notes[(s/12000)%8],pan=sinf((s/12000)*2.4f)*.4f;
        n*=1+.035f*(1-order)*sinf(t*13);
        float bell=(sinf(n*6.283185f*a)+.25f*sinf(n*12.57f*a))*expf(-a*13)*.17f;
        float pad=(sinf(t*461.29f)+.4f*sinf(t*691.94f))*.075f*fminf(1,t/4);
        pad+=sinf(t*580.86f)*.04f*fminf(1,fmaxf(0,t/16-2));
        float kick=sinf(270*b+15*(1-expf(-b*40)))*expf(-b*12)*.2f*order;
        random_state=random_state*1664525+1013904223;
        float hat=(int32_t)random_state/2147483648.f*expf(-a*80)*.035f*order;
        out[2*i]=(pad+kick+hat+bell*(1+pan))*ending;
        out[2*i+1]=(pad+kick+hat+bell*(1-pan))*ending;
    }
    atomic_store_explicit(&cursor,s,memory_order_release);
}
int demo(void){
    SDL_SetHint("SDL_VIDEODRIVER","x11");
    if(SDL_Init(0x31))return 1;
    void *w=SDL_CreateWindow("MORPH / 4K / PHASE LOCK",0x2fff0000,0x2fff0000,1920,1080,0x12);
    if(!w||!SDL_GL_CreateContext(w)){SDL_Quit();return 2;}
    SDL_GL_SetSwapInterval(1);SDL_ShowCursor(0);
    const char *s=fragment_shader;
    unsigned sh=glCreateShader(0x8b30);glShaderSource(sh,1,&s,0);glCompileShader(sh);
    unsigned p=glCreateProgram();glAttachShader(p,sh);glLinkProgram(p);glUseProgram(p);
    int loc=glGetUniformLocation(p,"u"),width,height;
    SDL_GL_GetDrawableSize(w,&width,&height);glViewport(0,0,width,height);
    AudioSpec a={.freq=48000,.format=0x8120,.channels=2,.samples=1024,.callback=audio};
    unsigned dev=SDL_OpenAudioDevice(0,0,&a,0,0);
    if(!dev){SDL_Quit();return 3;}
    SDL_PauseAudioDevice(dev,0);
    unsigned e[14];
    while(atomic_load_explicit(&cursor,memory_order_acquire)<3073024){
        while(SDL_PollEvent(e))if(e[0]==0x100||(e[0]==0x300&&e[5]==27))goto done;
        unsigned s=atomic_load_explicit(&cursor,memory_order_acquire);
        float t=(s>1024?s-1024:0)/48000.f;
        glUniform4f(loc,t,width,height,expf(-6*fmodf(t*2,1)));
        glRectf(-1,-1,1,1);SDL_GL_SwapWindow(w);SDL_Delay(1);
    }
done:
    SDL_CloseAudioDevice(dev);SDL_ShowCursor(1);SDL_Quit();return 0;
}
