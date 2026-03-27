// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <asm/desc/desc.h>

.text

.global asm_switch_to
.type asm_switch_to,@function
asm_switch_to:
    subq $16, %rsp
    movq %rsi,8(%rsp)
    movq %rdi,(%rsp)
    leaq asm_switch_context(%rip), %rax
    callq *%rax
    addq $16, %rsp
    retq

asm_switch_context:
    pushq %rdi
    pushq %rsi
    pushq %rbx
    pushq %rbp

    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    /* 栈切换 */
    movq 72(%rsp), %rax
    movq %rsp,(%rax)
    movq 80(%rsp), %rax
    movq (%rax), %rsp

    popq %r15
    popq %r14
    popq %r13
    popq %r12

    popq %rbp
    popq %rbx
    popq %rsi
    popq %rdi

    retq

.global asm_switch_to_user
.type asm_switch_to_user,@function
asm_switch_to_user:
    movq %rdi,%rsp

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

    addq $16,%rsp  // skip int_vector and error_code

    iretq