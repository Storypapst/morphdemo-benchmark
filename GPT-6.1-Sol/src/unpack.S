/* Native x86-64 self-decompression. No shell, child process, external packer,
 * or named temporary file is used. The production replaces this process via
 * execveat on an anonymous memfd. Every shared library is resolved normally.
 * Stream: flag byte (LSB first); 0 = literal; 1 = two-byte backreference.
 * Backreference: low 12 bits = distance - 1; high 4 bits = length - 3. */
.section .text
.global elf
elf:
    .byte 0x7f, 'E', 'L', 'F', 2, 1, 1, 0
    .quad 0
    .short 2, 62
    .long 1
    .quad 0x400000 + entry - elf
    .quad 64, 0
    .long 0
    .short 64, 56, 1, 0, 0, 0
    .long 1, 5
    .quad 0, 0x400000, 0x400000
    .quad end - elf, end - elf, 4096
entry:
    mov %rsp, %r13
    xor %edi, %edi
    mov $PAYLOAD_LENGTH, %esi
    mov $3, %edx
    mov $0x22, %r10d
    mov $-1, %r8
    xor %r9d, %r9d
    mov $9, %eax
    syscall
    cmp $-4095, %rax
    jae fail
    mov %rax, %r12
    mov %rax, %rdi
    lea PAYLOAD_LENGTH(%rax), %rdx
    lea packed(%rip), %rsi
group:
    lodsb
    movzbl %al, %ebx
    mov $8, %ebp
token:
    shr $1, %ebx
    jnc literal
    lodsw
    movzwl %ax, %ecx
    mov %ecx, %eax
    shr $12, %ecx
    add $3, %ecx
    and $4095, %eax
    inc %eax
    push %rsi
    mov %rdi, %rsi
    sub %rax, %rsi
    rep movsb
    pop %rsi
    jmp next
literal:
    movsb
next:
    cmp %rdx, %rdi
    je unpacked
    dec %ebp
    jnz token
    jmp group
unpacked:
    lea name(%rip), %rdi
    xor %esi, %esi
    mov $319, %eax
    syscall
    test %eax, %eax
    js fail
    mov %eax, %ebp
    mov %eax, %edi
    mov %r12, %rsi
    mov $PAYLOAD_LENGTH, %edx
write:
    mov $1, %eax
    syscall
    test %eax, %eax
    jle fail
    add %rax, %rsi
    sub %eax, %edx
    jnz write
    mov %ebp, %edi
    lea name+5(%rip), %rsi
    lea 8(%r13), %rdx
    mov (%r13), %rax
    lea 16(%r13,%rax,8), %r10
    mov $0x1000, %r8d
    mov $322, %eax
    syscall
fail:
    mov $126, %edi
    mov $231, %eax
    syscall
name:
    .asciz "MORPH"
packed:
    .incbin PAYLOAD_FILE
end:
.section .note.GNU-stack,"",@progbits
