/* Minimal linker script for a bare crt0.o (no Xilinx standalone BSP).
 * 32KB local BRAM at 0x0 (dlmb/ilmb_bram_if_cntlr, both alias the same
 * physical lmb_bram). Vector table layout matches crt0.o's fixed section
 * placement expectations (MicroBlaze standard: reset=0x0, sw_exception=0x8,
 * interrupt=0x10, hw_exception=0x20). */

MEMORY
{
    bram (rwx) : ORIGIN = 0x00000000, LENGTH = 0x8000
}

ENTRY(_start)

SECTIONS
{
    .vectors.reset 0x00000000 : { KEEP(*(.vectors.reset)) } > bram
    .vectors.sw_exception 0x00000008 : { KEEP(*(.vectors.sw_exception)) } > bram
    .vectors.interrupt 0x00000010 : { KEEP(*(.vectors.interrupt)) } > bram
    .vectors.hw_exception 0x00000020 : { KEEP(*(.vectors.hw_exception)) } > bram

    .text 0x00000028 : {
        *(.text*)
        *(.rodata*)
    } > bram

    . = ALIGN(4);
    _SDA2_BASE_ = .;
    .sdata2 : { *(.sdata2*) *(.sbss2*) } > bram

    . = ALIGN(4);
    _SDA_BASE_ = .;
    .sdata : { *(.sdata*) } > bram
    .sbss : { *(.sbss*) } > bram

    .data : { *(.data*) } > bram

    . = ALIGN(4);
    __bss_start = .;
    .bss : { *(.bss*) *(COMMON) } > bram
    . = ALIGN(4);
    __bss_end = .;

    . = ALIGN(8);
    _stack = 0x00008000 - 4;
}
