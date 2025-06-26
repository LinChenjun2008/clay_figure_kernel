/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#include <kernel/const.h>

.text

.extern kernel_main
.extern ap_kernel_main

.global kernel_entry
kernel_entry:
    movq $IA32_APIC_BASE, %rcx
    rdmsr
    andl $(1 << 8), %eax
    jz ap_kernel_main
    movq $KERNEL_PAGE_DIR_TABLE_POS , %rax
    movq %rax, %cr3
    movq $(KERNEL_VMA_BASE + KERNEL_STACK_BASE + KERNEL_STACK_SIZE), %rsp
    leaq kernel_main(%rip), %rax
    movq $KERNEL_TEXT_BASE, %rdi
    orq %rdi, %rax
    jmpq *%rax