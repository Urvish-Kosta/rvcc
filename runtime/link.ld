/* link.ld — bare-metal layout for the QEMU 'virt' machine.
   RAM base is 0x80000000; QEMU (-bios none -kernel elf) enters at _start.
   A small stack is reserved at the end of the image. The .tohost section is
   given a home so the HTIF adapter links cleanly when selected. */
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY {
    RAM (rwxa) : ORIGIN = 0x80000000, LENGTH = 16M
}

SECTIONS {
    . = 0x80000000;
    .text.init : { *(.text.init) }        > RAM
    .text      : { *(.text*) }            > RAM
    .rodata    : { *(.rodata*) }          > RAM
    .data      : { *(.data*) }            > RAM
    .tohost    : { *(.tohost) }           > RAM
    .bss       : { *(.bss*) *(COMMON) }   > RAM

    . = ALIGN(16);
    . = . + 0x1000;                       /* 4 KiB stack */
    __stack_top = .;
}
