#ifndef LOOM_PLATFORM_H
#define LOOM_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
typedef struct SDL_Window SDL_Window;
typedef void *SDL_GLContext;
typedef uint32_t SDL_AudioDeviceID;
typedef struct {int freq; uint16_t format; uint8_t channels,silence;uint16_t samples,padding;uint32_t size;void(*callback)(void*,uint8_t*,int);void *userdata;} SDL_AudioSpec;
typedef union { uint32_t type; struct {uint32_t type,timestamp,windowID;uint8_t state,repeat,pad1,pad2;struct{int scancode,sym;uint16_t mod;uint32_t unused;}keysym;} key;uint8_t padding[56];} SDL_Event;
int SDL_Init(uint32_t);void SDL_Quit(void);const char *SDL_GetError(void);
int SDL_GL_SetAttribute(int,int);SDL_Window *SDL_CreateWindow(const char*,int,int,int,int,uint32_t);void SDL_DestroyWindow(SDL_Window*);
SDL_GLContext SDL_GL_CreateContext(SDL_Window*);void SDL_GL_DeleteContext(SDL_GLContext);int SDL_GL_SetSwapInterval(int);void SDL_GL_SwapWindow(SDL_Window*);void SDL_GL_GetDrawableSize(SDL_Window*,int*,int*);
int SDL_PollEvent(SDL_Event*);uint64_t SDL_GetPerformanceCounter(void);uint64_t SDL_GetPerformanceFrequency(void);void SDL_Delay(uint32_t);int SDL_ShowCursor(int);
SDL_AudioDeviceID SDL_OpenAudioDevice(const char*,int,const SDL_AudioSpec*,SDL_AudioSpec*,int);void SDL_CloseAudioDevice(SDL_AudioDeviceID);int SDL_QueueAudio(SDL_AudioDeviceID,const void*,uint32_t);void SDL_PauseAudioDevice(SDL_AudioDeviceID,int);uint32_t SDL_GetQueuedAudioSize(SDL_AudioDeviceID);
typedef unsigned int GLenum,GLuint;typedef int GLint,GLsizei;typedef float GLfloat;typedef char GLchar;typedef unsigned char GLboolean;typedef unsigned int GLbitfield;
GLuint glCreateShader(GLenum);void glShaderSource(GLuint,GLsizei,const GLchar *const*,const GLint*);void glCompileShader(GLuint);void glGetShaderiv(GLuint,GLenum,GLint*);void glGetShaderInfoLog(GLuint,GLsizei,GLsizei*,GLchar*);GLuint glCreateProgram(void);void glAttachShader(GLuint,GLuint);void glLinkProgram(GLuint);void glGetProgramiv(GLuint,GLenum,GLint*);void glGetProgramInfoLog(GLuint,GLsizei,GLsizei*,GLchar*);void glUseProgram(GLuint);GLint glGetUniformLocation(GLuint,const GLchar*);void glUniform1f(GLint,GLfloat);void glUniform1i(GLint,GLint);void glUniform2f(GLint,GLfloat,GLfloat);void glGenVertexArrays(GLsizei,GLuint*);void glBindVertexArray(GLuint);void glDrawArrays(GLenum,GLint,GLsizei);void glViewport(GLint,GLint,GLsizei,GLsizei);void glReadPixels(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*);void glFinish(void);
void glGenTextures(GLsizei,GLuint*);void glBindTexture(GLenum,GLuint);void glTexImage2D(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);void glTexParameteri(GLenum,GLenum,GLint);void glActiveTexture(GLenum);void glGenFramebuffers(GLsizei,GLuint*);void glBindFramebuffer(GLenum,GLuint);void glFramebufferTexture2D(GLenum,GLenum,GLenum,GLuint,GLint);
#endif
