OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(kernel_entry)

SECTIONS
{
    . = 0xffff800000100000;
    _kernel_start = .;
    _text = .;
    .text :
    {
        kernel_entry = .;
        *(.text)
    }
    _etext = .;
    . = ALIGN(8);
    _data = .;
    .data :
    {
        *(.data)
    }
    _edata = .;
    _rodata = .;
    .rodata :
    {
        *(.rodata)
    }
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
