// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 Lin Chenjun
 */

#define INTR_HANDLER(ENTRY,NR,ERROR_CODE) \
.global ENTRY; \
.type ENTRY,@function; \
ENTRY:; \
    subq $ERROR_CODE,%rsp; \
    pushq $NR; \
    pushq %rax; \
    leaq save_all(%rip),%rax; \
    jmp *%rax; \

.text

#include <asm/intr/handler.h>

save_all:
    popq  %rax
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8

    pushq %rdi
    pushq %rsi
    pushq %rbp
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %rax

    xorq  %rax, %rax
    movw   %gs, %ax
    pushq %rax
    movw  %fs, %ax
    pushq %rax
    movw  %es, %ax
    pushq %rax
    movw  %ds, %ax
    pushq %rax

    movq %rsp, %rdi

    leaq interrupt_handler(%rip), %rax
    callq *%rax

    leaq schedule(%rip), %rax
    callq *%rax

.global intr_exit
.type intr_exit,@function
intr_exit:

    leaq softirq_handler(%rip), %rax
    callq *%rax

    popq %rax
    movw %ax, %ds
    popq %rax
    movw %ax, %es
    popq %rax
    movw %ax, %fs
    popq %rax
    movw %ax, %gs

    popq %rax
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rbp
    popq %rsi
    popq %rdi

    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15

    addq $16, %rsp
    iretq
