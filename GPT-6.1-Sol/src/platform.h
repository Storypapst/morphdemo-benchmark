#ifndef PLATFORM_H
#define PLATFORM_H
/* The small ABI subset of SDL 2 and desktop OpenGL used by the demos.
 * The declarations follow the public SDL 2 headers; no development packages
 * are needed. See https://wiki.libsdl.org/SDL2/SDL_AudioSpec. */
#include <stdint.h>
typedef struct {
    int freq;
    uint16_t format;
    uint8_t channels, silence;
    uint16_t samples, padding;
    uint32_t size;
    void (*callback)(void *, uint8_t *, int);
    void *userdata;
} AudioSpec;
extern int SDL_Init(uint32_t);
extern int SDL_SetHint(const char *, const char *);
extern void SDL_Quit(void);
extern const char *SDL_GetError(void);
extern void *SDL_CreateWindow(const char *, int, int, int, int, uint32_t);
extern void SDL_DestroyWindow(void *);
extern void *SDL_GL_CreateContext(void *);
extern void SDL_GL_DeleteContext(void *);
extern int SDL_GL_SetAttribute(int, int);
extern int SDL_GL_SetSwapInterval(int);
extern void SDL_GL_GetDrawableSize(void *, int *, int *);
extern void SDL_GL_SwapWindow(void *);
extern int SDL_PollEvent(void *);
extern uint32_t SDL_GetTicks(void);
extern void SDL_Delay(uint32_t);
extern int SDL_ShowCursor(int);
extern uint32_t SDL_OpenAudioDevice(const char *, int, const AudioSpec *, AudioSpec *, int);
extern void SDL_PauseAudioDevice(uint32_t, int);
extern void SDL_CloseAudioDevice(uint32_t);
extern unsigned glCreateShader(unsigned);
extern void glShaderSource(unsigned, int, const char *const *, const int *);
extern void glCompileShader(unsigned);
extern void glGetShaderiv(unsigned, unsigned, int *);
extern void glGetShaderInfoLog(unsigned, int, int *, char *);
extern unsigned glCreateProgram(void);
extern void glAttachShader(unsigned, unsigned);
extern void glLinkProgram(unsigned);
extern void glGetProgramiv(unsigned, unsigned, int *);
extern void glGetProgramInfoLog(unsigned, int, int *, char *);
extern void glUseProgram(unsigned);
extern int glGetUniformLocation(unsigned, const char *);
extern void glUniform4f(int, float, float, float, float);
extern void glRectf(float, float, float, float);
extern void glViewport(int, int, int, int);
extern void glFinish(void);
extern const unsigned char *glGetString(unsigned);
#endif
