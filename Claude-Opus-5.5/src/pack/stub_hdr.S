/* Minimal ELF64 header + single RWX PT_LOAD for the unpacker. */
    .section .hdr, "awx"
    .globl _start
ehdr:
    .byte 0x7f, 'E', 'L', 'F', 2, 1, 1, 0
    .quad 0
    .word 2, 0x3e
    .long 1
    .quad _start
    .quad phdr - ehdr
    .quad 0
    .long 0
    .word 64, 56, 1, 0, 0, 0
phdr:
    .long 1, 7
    .quad 0
    .quad ehdr, ehdr
    .quad _filesz
    .quad _memsz
    .quad 0x1000
_start:
    mov %rsp, %rdi
    call entry

    .section .blob, "aw"
    .globl blob
blob:
    .incbin BLOB
