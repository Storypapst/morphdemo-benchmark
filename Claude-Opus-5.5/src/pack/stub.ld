OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)
SECTIONS
{
    . = 0x10000;
    .all : {
        *(.hdr)
        *(.text*)
        *(.rodata*)
        *(.data*)
        *(.blob)
    }
    _filesz = . - 0x10000;
    .bss : { *(.bss*) *(COMMON) }
    _memsz = . - 0x10000;
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
