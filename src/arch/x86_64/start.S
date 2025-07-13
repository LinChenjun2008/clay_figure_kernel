// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright 2024-2025 LinChenjun
 */

#include <kernel/const.h>

#define __ASM_INCLUDE__
#include <device/cpu.h> // IA32_APIC_BASE
#include <mem/page.h>   // KERNEL_PAGE_DIR_TABLE_POS


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