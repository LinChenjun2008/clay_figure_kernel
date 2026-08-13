// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>
#include <asm/page.h>

.global kernel_entry
kernel_entry:
    // page table
    movq BI_PAGE_TABLE_POS(%rdi), %rax
    movq                    %rax, %cr3

    // kernel stack
    movq BI_STACK_PAGES(%rdi), %rax
    shlq                  $12, %rax
    addq  BI_STACK_BASE(%rdi), %rax
    movq     $KERNEL_VMA_BASE, %rcx
    addq                 %rcx, %rax
    movq                 %rax, %rsp

    leaq             main(%rip), %rax
    addq BI_RELOCATE_BASE(%rdi), %rax
    jmpq *%rax
