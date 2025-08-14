OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(kernel_entry)

SECTIONS
{
    . = 0x0000000000100000;
    kernel_entry = .;
    . += 0xffffffff80000000;
    _kernel_start = .;
    _text = .;
    .text :
    {
        *(.text)
    } :text
    _etext = .;

    . = ALIGN(8);

    _rodata = .;
    .rodata :
    {
        *(.rodata)
    } :rodata
    _erodata = .;

    _data = .;
    .data :
    {
        *(.data)
    } :data
    _edata = .;

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
    text   PT_LOAD FLAGS(5); /* RX (0x5) */
    rodata PT_LOAD FLAGS(4); /* R  (0x4) */
    data   PT_LOAD FLAGS(6); /* RW (0x6) */

}
