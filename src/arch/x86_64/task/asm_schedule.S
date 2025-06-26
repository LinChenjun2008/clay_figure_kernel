/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

.text

.global asm_switch_to
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


.extern intr_exit
.global asm_process_start
asm_process_start:
    movq %rdi, %rsp
    jmp intr_exit