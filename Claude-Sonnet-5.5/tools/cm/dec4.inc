; cm4 decoder (x86-64, needs BMI2 + SSE4.2). See tools/cm/cm.h for the model.
;   in:  rsi = compressed stream (followed by zero bytes), rdi = destination, r14d = destination end,
;        rbp = parameter block (NM mask bytes, then NM weight bytes), ebx = base of the slot table (zeroed memory)
;   uses 128 bytes of stack, clobbers everything else. Falls through when done.
dec4:
    mov   r15, 0x0101010101010101
    add   rsp, -128
    lodsd
    bswap eax
    mov   r10d, eax             ; code
    or    r9d, -1               ; range
    xor   r11d, r11d            ; hist
.byte:
    push  NM
    pop   rcx
.h: movzx eax, byte [rbp+rcx-1]
    pdep  rax, rax, r15
    imul  rax, rax, 255
    and   rax, r11
    mov   edx, ecx
    crc32 rdx, rax
    mov   [rsp+rcx*4-4], edx
    loop  .h
    push  1
    pop   r13                   ; c0
.bit:
    mov   r12d, ACC0
    push  NM
    pop   rcx
.p: mov   eax, [rsp+rcx*4-4]
    crc32 eax, r13b
    and   eax, TMASK
    mov   [rsp+rcx*4+60], eax
    mov   eax, [rbx+rax*4]
    movzx edx, byte [rbp+rcx+NM-1]
    imul  eax, edx
    add   r12d, eax
    loop  .p
    movzx eax, r12w             ; s0
    shr   r12d, 16              ; s1
    lea   ecx, [rax+r12]
    shl   eax, 16
    xor   edx, edx
    div   ecx                   ; p0 (16 bit)
    mov   ecx, r9d
    shr   ecx, 16
    imul  eax, ecx              ; bound
    xor   edx, edx
    cmp   r10d, eax
    jb    .z
    sub   r10d, eax
    sub   r9d, eax
    inc   edx                   ; y = 1
    jmp   short .n
.z: mov   r9d, eax
.n: test  r9d, 0xff000000
    jnz   .ok
    shl   r9d, 8
    shl   r10d, 8
    lodsb
    or    r10b, al
    jmp   short .n
.ok:
    push  1
    pop   r8
    sub   r8d, edx              ; 1 - y
    push  NM
    pop   rcx
.u: mov   eax, [rsp+rcx*4+60]
    lea   rax, [rbx+rax*4]
    cmp   byte [rax+rdx*2], CAP
    adc   byte [rax+rdx*2], 0
    cmp   byte [rax+r8*2], NS
    jbe   .k
    shr   byte [rax+r8*2], 1
.k: loop  .u
    lea   r13d, [rdx+r13*2]
    bt    r13d, 8
    jnc   .bit
    mov   eax, r13d
    stosb
    shl   r11, 8
    or    r11b, al
    cmp   edi, r14d
    jb    .byte
    sub   rsp, -128
