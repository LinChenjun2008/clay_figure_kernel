OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(kernel_entry)

SECTIONS
{
    KERNEL_VMA_BASE = 0xffff800000000000;
    . = 0x0000000000100000;
    . += KERNEL_VMA_BASE;
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
