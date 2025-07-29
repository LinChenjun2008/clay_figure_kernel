// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/const.h>

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

.extern do_irq
.extern do_softirq
.extern schedule

#include <intr.h>

save_all:
    popq %rax
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
    movw  %gs, %ax
    pushq %rax
    movw  %fs, %ax
    pushq %rax
    movw  %es, %ax
    pushq %rax
    movw  %ds, %ax
    pushq %rax

    movq %rsp, %rdi

    leaq do_irq(%rip), %rax
    callq *%rax

    leaq schedule(%rip),%rax
    callq *%rax

.global intr_exit
.type intr_exit,@function
intr_exit:
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

    addq $16, %rsp // error code,nr
    iretq

.global asm_debug_intr
.type asm_debug_intr, @function
asm_debug_intr:
    int $0x82
    ret