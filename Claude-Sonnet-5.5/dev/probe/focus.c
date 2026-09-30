// Focus probe (development only): does the fullscreen window become the active window by itself, or after SDL_RaiseWindow / SDL_SetWindowInputFocus?
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
int SDL_Init(uint32_t);
void *SDL_CreateWindow(const char*, int, int, int, int, uint32_t);
void *SDL_GL_CreateContext(void*);
void SDL_GL_SwapWindow(void*);
int SDL_PollEvent(void*);
uint32_t SDL_GetTicks(void);
void SDL_RaiseWindow(void*);
int SDL_SetWindowInputFocus(void*);
int SDL_SetWindowFullscreen(void*,uint32_t);
void SDL_ShowWindow(void*);
uint32_t SDL_GetWindowFlags(void*);
int main(int argc,char**argv){
  int mode=argc>1?atoi(argv[1]):0;
  SDL_Init(0x20);
  void *w=SDL_CreateWindow("focus",0x1fff0000,0x1fff0000,mode&8?1280:0,mode&8?720:0,mode&8?0x2:0x1003);
  SDL_GL_CreateContext(w);
  if(mode&1) SDL_RaiseWindow(w);
  if(mode&2) SDL_SetWindowInputFocus(w);
  if(mode&4) SDL_ShowWindow(w);
  uint32_t t0=SDL_GetTicks(); uint8_t ev[64]; int fsdone=0;
  while(SDL_GetTicks()-t0<5000){
    if((mode&16)&&!fsdone&&SDL_GetTicks()-t0>1500){ fsdone=1; SDL_SetWindowFullscreen(w,0x1001); fprintf(stderr,"[%u ms] requested fullscreen\n",SDL_GetTicks()-t0); } while(SDL_PollEvent(ev)){ uint32_t ty=*(uint32_t*)ev; if(ty==0x200) fprintf(stderr,"[%u ms] window event %d\n",SDL_GetTicks()-t0,*(int*)(ev+12)); if(ty==0x300) fprintf(stderr,"key %d\n",*(int*)(ev+20)); } SDL_GL_SwapWindow(w); usleep(5000); }
  fprintf(stderr,"flags at end: 0x%x (INPUT_FOCUS=0x200)\n",SDL_GetWindowFlags(w));
  return 0;
}
