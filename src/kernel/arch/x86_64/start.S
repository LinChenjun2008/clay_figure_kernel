#include <kernel/const.h>

.extern kernel_main
.extern ap_kernel_main

.global kernel_entry
kernel_entry:
    movq $IA32_APIC_BASE,%rcx
    rdmsr
    andl $(1 << 8),%eax
    jz ap_kernel_main
    movq $0x5f9000,%rax
    movq %rax,%cr3
    movq $0xffff800000310000,%rsp
    jmp kernel_main