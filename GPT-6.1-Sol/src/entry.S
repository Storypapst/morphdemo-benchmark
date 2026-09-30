/* A C entry point without a C runtime startup object. The dynamic loader has
 * already initialized libc and the shared libraries. */
.text
.global _start
.type _start,@function
_start:
    xor %ebp,%ebp
    and $-16,%rsp
    call demo
    mov %eax,%edi
    mov $231,%eax
    syscall
.section .note.GNU-stack,"",@progbits
