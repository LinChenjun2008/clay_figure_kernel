#include <kernel/const.h>

.extern kernel_main
.extern ap_kernel_main
.global kernel_entry
kernel_entry:
    movq $IA32_APIC_BASE,%rcx
    rdmsr
    andl $(1 << 8),%eax
    jnz kernel_main
    jmp ap_kernel_main