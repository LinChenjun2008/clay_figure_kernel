// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <task/struct.h>

.text

.extern intr_exit

.global asm_get_task_mgr
.type asm_get_task_mgr,@function
asm_get_task_mgr:
    swapgs
    movq %gs:CPU_TASK_MGR, %rax
    swapgs
    retq

.global asm_switch_to
.type asm_switch_to,@function
asm_switch_to:
    subq                      $16, %rsp
    movq                     %rsi, 8(%rsp)
    movq                     %rdi, (%rsp)
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
    movq     %rsp, (%rax)
    movq 80(%rsp), %rax
    movq   (%rax), %rsp

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
    movq            %rdi, %rsp
    leaq intr_exit(%rip), %rax
    jmpq *%rax