// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#include <task/task.h>

.global send_recv
.type send_recv,@function
send_recv:
    xchgq %rcx, %r10 // save rcx in r10
    syscall
    retq

.global syscall_entry
.type syscall_entry,@function
syscall_entry:
    // user stack
    pushq %rbp
    pushq %rcx       // origin rip
    pushq %r11       // origin rflags
    xchgq %rcx, %r10 // restore rcx

    // switch to kernel stack
    swapgs
    movq %gs:TASK_STRUCT_KSTACK_BASE,%rax
    addq %gs:TASK_STRUCT_KSTACK_SIZE,%rax
    swapgs

    movq %rsp, %rbp
    movq %rax, %rsp

    pushq %rbp // save rbp (user stack)

    sti

    leaq sys_send_recv(%rip), %rax
    callq *%rax

    cli

    // switch to user stack
    popq %rbp
    movq %rbp, %rsp

    popq %r11
    popq %rcx
    popq %rbp

    sysretq
