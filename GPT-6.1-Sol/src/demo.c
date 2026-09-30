#include "platform.h"
#include <stdio.h>
#include "music.h"
#include "shader.h"

int demo(void) {
    SDL_SetHint("SDL_VIDEODRIVER","x11");
    if(SDL_Init(0x31)) { fprintf(stderr,"SDL: %s\n",SDL_GetError()); return 1; }
    SDL_GL_SetAttribute(17,3); SDL_GL_SetAttribute(18,3);
    SDL_GL_SetAttribute(21,2); SDL_GL_SetAttribute(5,1);
#if DEMO == 64
    const char *title="MORPH / 64K / CHORUS OF MATTER";
#else
    const char *title="MORPH / 16K / LATTICE OF LIGHT";
#endif
    void *window=SDL_CreateWindow(title,0x2fff0000,0x2fff0000,1920,1080,0x12);
    void *context=window?SDL_GL_CreateContext(window):0;
    if(!context) { fprintf(stderr,"OpenGL: %s\n",SDL_GetError()); SDL_Quit(); return 2; }
    SDL_GL_SetSwapInterval(1); SDL_ShowCursor(0);
    unsigned shader=glCreateShader(0x8b30);
    const char *source=fragment_shader;
    glShaderSource(shader,1,&source,0); glCompileShader(shader);
    int status=0;
    glGetShaderiv(shader,0x8b81,&status);
    char log[4096];
    if(!status) { glGetShaderInfoLog(shader,sizeof log,0,log); fprintf(stderr,"Shader: %s\n",log); SDL_Quit(); return 3; }
    unsigned program=glCreateProgram();
    glAttachShader(program,shader); glLinkProgram(program);
    glGetProgramiv(program,0x8b82,&status);
    if(!status) { glGetProgramInfoLog(program,sizeof log,0,log); fprintf(stderr,"Program: %s\n",log); SDL_Quit(); return 4; }
    glUseProgram(program);
    int uniform=glGetUniformLocation(program,"u"),w,h;
    SDL_GL_GetDrawableSize(window,&w,&h); glViewport(0,0,w,h);
    prepare_music();
    AudioSpec wanted={.freq=RATE,.format=0x8120,.channels=2,.samples=1024,.callback=audio};
    uint32_t device=SDL_OpenAudioDevice(0,0,&wanted,0,0);
    if(!device) { fprintf(stderr,"Audio: %s\n",SDL_GetError()); SDL_Quit(); return 5; }
    fprintf(stderr,"%s | %dx%d | GL %s | 48000 Hz stereo | 64 s\n",title,w,h,glGetString(0x1f02));
    uint32_t event[14],start=SDL_GetTicks();
    SDL_PauseAudioDevice(device,0);
    unsigned frames=0;
    int run=1;
    while(run) {
        while(SDL_PollEvent(event)) if(event[0]==0x100 || (event[0]==0x300 && event[5]==27)) run=0;
        unsigned s=atomic_load_explicit(&sample_cursor,memory_order_acquire);
        /* The audio sample cursor is the master clock. Subtract one callback
         * block to account for the device buffer currently being played. */
        float t=(s>1024?s-1024:0)/(float)RATE;
        if(t>=64 || !run) break;
        glUniform4f(uniform,t,(float)w,(float)h,expf(-6*fmodf(t*2,1)));
        glRectf(-1,-1,1,1); SDL_GL_SwapWindow(window); frames++;
        SDL_Delay(1);
    }
    SDL_CloseAudioDevice(device);
    fprintf(stderr,"Ended cleanly: %.3f s, %u frames\n",(SDL_GetTicks()-start)/1000.f,frames);
    SDL_ShowCursor(1); SDL_GL_DeleteContext(context); SDL_DestroyWindow(window); SDL_Quit();
    return 0;
}
