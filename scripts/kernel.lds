OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(kernel_entry)

SECTIONS
{
    _kernel_start = .;

    _text = .;
    .text :
    {
        kernel_entry = .;
        *(.text .text.*)
    } :text
    _etext = .;

    . = ALIGN(8);

    _rodata = .;
    .rodata :
    {
        *(.rodata .rodata.*)
    } :rodata
    _erodata = .;

    . = ALIGN(8);

    .dynamic :
    {
        *(.dynamic)
    } :rodata :dynamic

    . = ALIGN(8);

    _data = .;
    .data :
    {
        *(.data .data.*)
    } :data
    _edata = .;

    . = ALIGN(8);

    _bss = .;
    .bss :
    {
        *(.bss .bss.*)
    }
    _ebss = .;

    _kernel_end = .;
    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.note .note.*)
    }
}

PHDRS
{
    text    PT_LOAD    FLAGS(5); /* RX (0x5) */
    rodata  PT_LOAD    FLAGS(4); /* R  (0x4) */
    data    PT_LOAD    FLAGS(6); /* RW (0x6) */
    dynamic PT_DYNAMIC FLAGS(4); /* R  (0x4) */
}
