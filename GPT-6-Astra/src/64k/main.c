#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "music.c"
#include "shaders.h"
static const char *vertex="#version 330 core\nvoid main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0,1);}";
static GLuint shader(GLenum type,const char *source){GLuint s=glCreateShader(type);glShaderSource(s,1,&source,0);glCompileShader(s);GLint ok;glGetShaderiv(s,0x8B81,&ok);if(!ok){char log[8192];glGetShaderInfoLog(s,sizeof(log),0,log);fprintf(stderr,"THE LOOM shader: %s\n",log);exit(2);}return s;}
static GLuint program(const char *frag){GLuint p=glCreateProgram();glAttachShader(p,shader(0x8B31,vertex));glAttachShader(p,shader(0x8B30,frag));glLinkProgram(p);GLint ok;glGetProgramiv(p,0x8B82,&ok);if(!ok){char log[8192];glGetProgramInfoLog(p,sizeof(log),0,log);fprintf(stderr,"THE LOOM link: %s\n",log);exit(2);}return p;}
static int capture(const char *name,int w,int h){
 unsigned char *pixels=malloc((size_t)w*h*3);if(!pixels)return 1;glFinish();glReadPixels(0,0,w,h,0x1907,0x1401,pixels);
 FILE *f=fopen(name,"wb");if(!f){free(pixels);return 1;}fprintf(f,"P6\n%d %d\n255\n",w,h);
 for(int y=h-1;y>=0;y--)fwrite(pixels+(size_t)y*w*3,1,(size_t)w*3,f);fclose(f);free(pixels);return 0;
}
int main(int argc,char **argv){
 int windowed=0,mute=0;float fixed=-1.f;const char *still=0,*wave=0;
 for(int i=1;i<argc;i++){
  if(!strcmp(argv[i],"--windowed"))windowed=1;
  else if(!strcmp(argv[i],"--mute"))mute=1;
  else if(!strcmp(argv[i],"--time")&&i+1<argc)fixed=strtof(argv[++i],0);
  else if(!strcmp(argv[i],"--capture")&&i+1<argc)still=argv[++i];
  else if(!strcmp(argv[i],"--audio")&&i+1<argc)wave=argv[++i];
  else{fprintf(stderr,"Usage: %s [--windowed] [--mute] [--time seconds --capture image.ppm] [--audio score.wav]\n",argv[0]);return 1;}
 }
 if(wave){if(!compose())return 1;int status=write_wave(wave);free(song);return status;}
 if(still){mute=1;windowed=1;if(fixed<0)fixed=40.f;}
 if(SDL_Init(0x20|(mute?0:0x10))){fprintf(stderr,"SDL: %s\n",SDL_GetError());return 1;}
 SDL_GL_SetAttribute(17,3);SDL_GL_SetAttribute(18,3);SDL_GL_SetAttribute(21,1);SDL_GL_SetAttribute(5,1);
 SDL_Window *window=SDL_CreateWindow("MORPH / THE LOOM",0x2FFF0000,0x2FFF0000,1920,1080,2|(windowed?4:0x1001));
 if(!window){fprintf(stderr,"Window: %s\n",SDL_GetError());SDL_Quit();return 1;}
 SDL_GLContext context=SDL_GL_CreateContext(window);if(!context){fprintf(stderr,"OpenGL: %s\n",SDL_GetError());SDL_DestroyWindow(window);SDL_Quit();return 1;}
 SDL_GL_SetSwapInterval(1);SDL_ShowCursor(0);
 GLuint vao,texture,fbo;glGenVertexArrays(1,&vao);glBindVertexArray(vao);
 GLuint scene=program(scene_shader),post=program(post_shader);
 glGenTextures(1,&texture);glBindTexture(0x0DE1,texture);glTexImage2D(0x0DE1,0,0x881A,1920,1080,0,0x1908,0x1406,0);
 glTexParameteri(0x0DE1,0x2801,0x2601);glTexParameteri(0x0DE1,0x2800,0x2601);glTexParameteri(0x0DE1,0x2802,0x812F);glTexParameteri(0x0DE1,0x2803,0x812F);
 glGenFramebuffers(1,&fbo);glBindFramebuffer(0x8D40,fbo);glFramebufferTexture2D(0x8D40,0x8CE0,0x0DE1,texture,0);
 SDL_AudioDeviceID audio=0;
 if(!mute){
  SDL_AudioSpec want={.freq=RATE,.format=0x8120,.channels=2,.samples=1024};
  audio=SDL_OpenAudioDevice(0,0,&want,0,0);
  if(!audio){fprintf(stderr,"Audio: %s\n",SDL_GetError());SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();return 1;}
  if(!compose()||SDL_QueueAudio(audio,song,NSAMPLES*8)){fprintf(stderr,"Audio allocation/queue failed\n");SDL_CloseAudioDevice(audio);SDL_Quit();return 1;}
  free(song);
 }
 uint64_t start=SDL_GetPerformanceCounter(),freq=SDL_GetPerformanceFrequency();if(audio)SDL_PauseAudioDevice(audio,0);
 int running=1,status=0;
 while(running){
  SDL_Event event;while(SDL_PollEvent(&event))if(event.type==0x100||(event.type==0x300&&event.key.keysym.sym==27))running=0;
  float t=fixed>=0?fixed:(float)((double)(SDL_GetPerformanceCounter()-start)/freq);
  if(!running||t>=60.f)break;
  int w,h;SDL_GL_GetDrawableSize(window,&w,&h);
  glBindFramebuffer(0x8D40,fbo);glViewport(0,0,1920,1080);glUseProgram(scene);glUniform1f(glGetUniformLocation(scene,"T"),t);glUniform2f(glGetUniformLocation(scene,"R"),1920,1080);glDrawArrays(4,0,3);
  glBindFramebuffer(0x8D40,0);glViewport(0,0,w,h);glUseProgram(post);glActiveTexture(0x84C0);glBindTexture(0x0DE1,texture);glUniform1i(glGetUniformLocation(post,"S"),0);glUniform1f(glGetUniformLocation(post,"T"),t);glUniform2f(glGetUniformLocation(post,"R"),w,h);glDrawArrays(4,0,3);
  if(still){status=capture(still,w,h);break;}
  SDL_GL_SwapWindow(window);
 }
 if(audio)SDL_CloseAudioDevice(audio);SDL_ShowCursor(1);SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();return status;
}
