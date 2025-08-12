OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(kernel_entry)

SECTIONS
{
    . = 0xffffffff80100000;
    _kernel_start = .;
    _text = .;
    .text :
    {
        kernel_entry = .;
        *(.text)
    } :text
    _etext = .;
    . = ALIGN(8);
    _data = .;
    .data :
    {
        *(.data)
    } :data
    _edata = .;
    _rodata = .;
    .rodata :
    {
        *(.rodata)
    } :rodata
    _erodata = .;
    _bss = .;
    .bss :
    {
        *(.bss)
    }
    _ebss = .;
    _kernel_end = .;
    /DISCARD/ :
    {
        *(.eh_frame)
    }
}

PHDRS
{
    text PT_LOAD FLAGS(5);   /* RX (0x5) */
    data PT_LOAD FLAGS(6);   /* RW (0x6) */
    rodata PT_LOAD FLAGS(4); /* R  (0x4) */
}
