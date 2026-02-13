// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 Lin Chenjun
 */

#include <kernel/const.h>

#include <device/cpu.h> // IA32_APIC_BASE
#include <mem/page.h>   // KERNEL_PAGE_DIR_TABLE_POS

.text

.extern kernel_main
.global kernel_entry
kernel_entry:
    movq $KERNEL_PAGE_DIR_TABLE_POS , %rax
    movq %rax, %cr3
    movq $(KERNEL_VMA_BASE + KERNEL_STACK_BASE + KERNEL_STACK_SIZE), %rsp
    leaq kernel_main(%rip), %rax
    addq $KERNEL_TEXT_BASE, %rax
    jmpq *%rax