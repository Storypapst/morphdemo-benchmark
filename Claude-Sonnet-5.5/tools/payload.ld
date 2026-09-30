/* Flat payload: code, read-only and initialised data in one block starting at DEST, zero-initialised data at BSS. */
ENTRY(entry)
SECTIONS {
  . = DEST;
  .text : { *(.text.entry) *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }
  .data : { *(.data .data.*) }
  payload_end = .;
  . = BSS;
  .bss (NOLOAD) : { *(.bss .bss.*) *(COMMON) }
  /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.gnu.*) *(.interp) *(.dynamic) }
}
