// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <asm/desc/desc.h>
#include <task.h>

.text

.global syscall
.type syscall,@function
syscall:
    pushq %rcx
    pushq %r11
    syscall
    popq %r11
    popq %rcx
    retq

.global asm_syscall_entry
.type asm_syscall_entry,@function
asm_syscall_entry:

    // user stack
    pushq %rbp       // save user rbp in user stack
    pushq %rax       // save user rax in user stack

    // user stack:
    //  rcx  24(rbp)
    //  r11  16(rbp)
    //  rbp   8(rbp)
    //  rax <--(rbp)

    // switch to kernel stack
    swapgs
    movq %gs:TASK_STRUCT_KSTACK_PAGES,%rax
    shlq $12,%rax
    addq %gs:TASK_STRUCT_KSTACK_BASE,%rax
    swapgs

    movq %rsp,%rbp // save user stack in rbp
    movq %rax,%rsp // switch to kernel stack

    // kernel stack

    // make pt_regs struct
    xorq %rax,%rax
    movw %ss,%ax
    pushq %rax                  // ss
    pushq %rbp                  // user stack
    pushq %r11                  // origin rflags saved in r11
    pushq $SELECTOR_USER_CODE64 // user cs selector
    pushq %rcx                  // origin rip saved in rcx
    pushq $0                    // error_code is always 0

    pushq $0                    // int_vector is always 0

    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq 16(%rbp)  // r11
    pushq %r10
    pushq %r9
    pushq %r8

    pushq %rdi
    pushq %rsi
    pushq 8(%rbp)   // rbp
    pushq %rdx
    pushq 24(%rbp)  // rcx
    pushq %rbx
    pushq (%rbp)

    xorq  %rax, %rax
    movw  %gs, %ax
    pushq %rax
    movw  %fs, %ax
    pushq %rax
    movw  %es, %ax
    pushq %rax
    movw  %ds, %ax
    pushq %rax

    movq %rsp,%rdi

    sti
    leaq syscall_entry(%rip),%rax
    callq *%rax
    cli

    // restore registers
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
    popq %rcx      // origin rip
    addq $8,%rsp   // skip user cs
    popq %r11      // origin rflags
    popq %rbp      // user stack
    // skip user ss

    // switch to user stack
    movq %rbp,%rsp

    // user stack
    // restore user rbp
    addq $8,%rsp // skip rax (return value)
    popq %rbp

    sysretq