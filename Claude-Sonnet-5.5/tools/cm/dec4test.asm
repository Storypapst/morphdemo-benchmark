; static test ELF: decode the stream, write the result to stdout
bits 64
BASE equ 0x400000
org BASE
%include "params.inc"
OUTLEN equ INLEN_PLACEHOLDER
TABLE equ 0x1000000
DSTBUF equ 0x800000
ehdr:
  db 0x7f,'ELF',2,1,1,0
  times 8 db 0
  dw 2, 0x3e
  dd 1
  dq start
  dq phdr - ehdr
  dq 0
  dd 0
  dw 64, 56, 1, 0, 0, 0
phdr:
  dd 1, 7
  dq 0, BASE, BASE, filesize, 0x8000000, 0x1000
start:
  mov  ebx, TABLE
  lea  rbp, [rel params]
  lea  rsi, [rel stream]
  mov  edi, DSTBUF
  mov  r14d, DSTBUF+OUTLEN
  call dec4
  ; write(1, DSTBUF, OUTLEN)
  mov  eax, 1
  mov  edi, 1
  mov  esi, DSTBUF
  mov  edx, OUTLEN
  syscall
  mov  eax, 231
  xor  edi, edi
  syscall
%include "dec4.inc"
  ret
params: PARAMS
stream: incbin "STREAMFILE"
  times 16 db 0
filesize equ $ - ehdr
