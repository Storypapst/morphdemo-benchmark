; morphdemo-4k payload: runs after the stub has unpacked it to DEST.
; Flow: resolve imports with dlsym -> SDL window + GL context -> synthesise 60 s of audio on the GPU (compute shader)
;       -> queue it -> render the fragment shader until 60 s have passed or Escape is pressed.
bits 64
%ifndef DEST
%define DEST 0x401000
%endif
%ifndef GOT
%define GOT 0x400028
%endif
org DEST
PTAB    equ 0x440000                       ; import table in the zero-filled tail (tools/pack.py checks that the payload ends below it)
NSAMP   equ 2880000
NBYTES  equ NSAMP*8
GL_CS   equ 0x91B9
GL_FS   equ 0x8B30
GL_SSBO equ 0x90D2

; import indices
%assign k 0
%macro IMPORT 1
  I_%1 equ k*8
  %assign k k+1
%endmacro
IMPORT SDL_Init
IMPORT SDL_CreateWindow
IMPORT SDL_GL_CreateContext
IMPORT glCreateShaderProgramv
IMPORT glUseProgram
IMPORT glBindBufferBase
IMPORT glBufferData
IMPORT glDispatchCompute
IMPORT glMapBuffer
IMPORT glMemoryBarrier
IMPORT SDL_OpenAudioDevice
IMPORT SDL_QueueAudio
IMPORT SDL_PauseAudioDevice
IMPORT SDL_GetTicks
IMPORT SDL_GL_GetDrawableSize
IMPORT glViewport
IMPORT SDL_PollEvent
IMPORT glUniform3i
IMPORT glRects
IMPORT SDL_GL_SwapWindow
IMPORT SDL_GL_SetSwapInterval
IMPORT SDL_ShowCursor

; Debug aid: assemble with -DDBG and every MARK prints one letter to stderr, which shows how far the start-up got. Without -DDBG it produces no code.
%macro MARK 1
%ifdef DBG
  push rax
  push rdi
  push rsi
  push rdx
  push rcx
  push r11
  mov byte [0x404000], %1
  mov eax, 1
  mov edi, 2
  mov esi, 0x404000
  mov edx, 1
  syscall
  pop r11
  pop rcx
  pop rdx
  pop rsi
  pop rdi
  pop rax
%endif
%endmacro

entry:
    ; ---- watchdog: SIGALRM ends the process if the production ever stalls ----
    mov   eax, 37
    push  67
    pop   rdi
    syscall
    ; ---- resolve imports ----
    sub   rsp, 8+64+16                 ; align (entered with rsp%16==8), event buffer at [rsp], viewport at [rsp+64]
    mov   ebx, PTAB
    mov   r12d, names
    xor   r13d, r13d
.res:
    xor   edi, edi
    mov   rsi, r12
    call  [GOT]
    mov   [rbx+r13*8], rax
    inc   r13
.skip:
    inc   r12
    cmp   byte [r12-1], 0
    jnz   .skip
    cmp   byte [r12], 0
    jnz   .res
    ; ---- window + context ----
    push  0x30
    pop   rdi
    call  [rbx+I_SDL_Init]
    MARK 'a'
    mov   edi, title                    ; window title
    mov   esi, 0x1fff0000
    mov   edx, esi
    xor   ecx, ecx
    xor   r8d, r8d
    mov   r9d, 0x1003
    call  [rbx+I_SDL_CreateWindow]
    MARK 'b'
    mov   r15, rax
    mov   rdi, rax
    call  [rbx+I_SDL_GL_CreateContext]
    MARK 'c'
    push  1
    pop   rdi
    call  [rbx+I_SDL_GL_SetSwapInterval]
    xor   edi, edi
    call  [rbx+I_SDL_ShowCursor]
    ; ---- audio: compute program ----
    mov   edx, strp_aud
    push  2
    pop   rsi
    mov   edi, GL_CS
    call  [rbx+I_glCreateShaderProgramv]
    MARK 'e'
    mov   edi, eax
    call  [rbx+I_glUseProgram]
    MARK 'f'
    mov   edi, GL_SSBO
    xor   esi, esi
    push  1
    pop   rdx
    call  [rbx+I_glBindBufferBase]
    mov   edi, GL_SSBO
    mov   esi, NBYTES
    xor   edx, edx
    mov   ecx, 0x88E9
    call  [rbx+I_glBufferData]
    MARK 'g'
    mov   edi, NSAMP/64
    push  1
    pop   rsi
    push  1
    pop   rdx
    call  [rbx+I_glDispatchCompute]
    MARK 'h'
    mov   edi, 0x200                   ; GL_BUFFER_UPDATE_BARRIER_BIT: make the shader writes visible to the mapping
    call  [rbx+I_glMemoryBarrier]
    mov   edi, GL_SSBO
    mov   esi, 0x88B8
    call  [rbx+I_glMapBuffer]
    MARK 'i'
    mov   r12, rax
    ; ---- audio device ----
    xor   eax, eax
    push  rax
    push  rax
    push  rax
    mov   rax, 0x000281200000BB80      ; 48000 Hz, AUDIO_F32, 2 channels
    push  rax
    xor   edi, edi
    xor   esi, esi
    mov   rdx, rsp
    xor   ecx, ecx
    xor   r8d, r8d
    call  [rbx+I_SDL_OpenAudioDevice]
    MARK 'j'
    add   rsp, 32
    mov   r13d, eax
    mov   edi, eax
    mov   rsi, r12
    mov   edx, NBYTES
    call  [rbx+I_SDL_QueueAudio]
    MARK 'k'
    ; ---- visual program ----
    mov   edx, strp_vis
    push  2
    pop   rsi
    mov   edi, GL_FS
    call  [rbx+I_glCreateShaderProgramv]
    mov   edi, eax
    call  [rbx+I_glUseProgram]
    ; ---- go: the first frame only warms the shader up, then the audio and the clock start together ----
    xor   r14d, r14d
.frame:
    call  [rbx+I_SDL_GetTicks]
    sub   eax, r14d
    mov   r12d, eax
    cmp   eax, 60000
    jae   .done
.ev:
    mov   rdi, rsp
    call  [rbx+I_SDL_PollEvent]
    test  eax, eax
    jz    .draw
    cmp   dword [rsp], 0x100
    je    .done
    cmp   dword [rsp], 0x300
    jne   .ev
    cmp   dword [rsp+20], 27
    jne   .ev
    jmp   .done
.draw:
    mov   rdi, r15
    lea   rsi, [rsp+64]
    lea   rdx, [rsp+68]
    call  [rbx+I_SDL_GL_GetDrawableSize]
    xor   edi, edi
    xor   esi, esi
    mov   edx, [rsp+64]
    mov   ecx, [rsp+68]
    call  [rbx+I_glViewport]
    xor   edi, edi
    mov   esi, r12d
    mov   edx, [rsp+64]
    mov   ecx, [rsp+68]
    call  [rbx+I_glUniform3i]
    or    edi, -1
    or    esi, -1
    push  1
    pop   rdx
    push  1
    pop   rcx
    call  [rbx+I_glRects]
    mov   rdi, r15
    call  [rbx+I_SDL_GL_SwapWindow]
    test  r14d, r14d
    jnz   .frame
    mov   edi, r13d
    xor   esi, esi
    call  [rbx+I_SDL_PauseAudioDevice]
    call  [rbx+I_SDL_GetTicks]
    mov   r14d, eax
    jmp   .frame
.done:
    xor   edi, edi
    mov   eax, 231
    syscall

title: db "MORPH",0
names:
    db "SDL_Init",0
    db "SDL_CreateWindow",0
    db "SDL_GL_CreateContext",0
    db "glCreateShaderProgramv",0
    db "glUseProgram",0
    db "glBindBufferBase",0
    db "glBufferData",0
    db "glDispatchCompute",0
    db "glMapBuffer",0
    db "glMemoryBarrier",0
    db "SDL_OpenAudioDevice",0
    db "SDL_QueueAudio",0
    db "SDL_PauseAudioDevice",0
    db "SDL_GetTicks",0
    db "SDL_GL_GetDrawableSize",0
    db "glViewport",0
    db "SDL_PollEvent",0
    db "glUniform3i",0
    db "glRects",0
    db "SDL_GL_SwapWindow",0
    db "SDL_GL_SetSwapInterval",0
    db "SDL_ShowCursor",0
    db 0
strp_aud: dq pre, aud
strp_vis: dq pre, vis
pre: incbin PRE_FILE
    db 0
vis: incbin VIS_FILE
    db 0
aud: incbin AUD_FILE
    db 0
payload_end:
