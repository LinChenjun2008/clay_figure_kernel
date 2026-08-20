// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>
#include <asm/page.h>

.global kernel_entry
kernel_entry:
    // rdi -> system_info
    // rsi -> boot_info
    movq (%rdi), %rsi

    // page table
    movq BI_PAGE_TABLE_POS(%rsi), %rax
    movq                    %rax, %cr3

    // kernel stack
    movq BI_STACK_PAGES(%rsi), %rax
    shlq                  $12, %rax
    addq  BI_STACK_BASE(%rsi), %rax
    movq     $KERNEL_VMA_BASE, %rcx
    addq                 %rcx, %rax
    movq                 %rax, %rsp

    leaq             main(%rip), %rax
    addq BI_RELOCATE_BASE(%rsi), %rax
    jmpq *%rax
