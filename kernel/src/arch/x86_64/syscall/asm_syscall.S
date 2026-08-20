// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <asm/desc.h>
#include <asm/page.h>
#include <asm/ptrace.h>

#include <syscall.h>
#include <task/struct.h>

.text

.global asm_exit
.type asm_exit,@function
asm_exit:
    movq     %rdi, %rsi
    movq $NR_EXIT, %rdi
    syscall
    retq

.global asm_syscall_entry
.type asm_syscall_entry,@function
asm_syscall_entry:

    swapgs
    movq %gs:CPU_CURRENT_TASK, %rax
    swapgs

    // save user sp
    movq %rsp, TASK_STRUCT_USTACK_SP(%rax)

    // switch to kernel stack
    movq TASK_STRUCT_KSTACK_PAGES(%rax), %rsp
    shlq               $PAGE_SIZE_SHIFT, %rsp
    addq  TASK_STRUCT_KSTACK_BASE(%rax), %rsp

    movq TASK_STRUCT_USTACK_SP(%rax), %rax

    // make pt_regs structure
    subq $REGS_SIZE, %rsp

    movq $SELECTOR_USER_DATA64, REG_SS(%rsp)
    movq                  %rax, REG_RSP(%rsp)
    movq                  %r11, REG_RFLAGS(%rsp)
    movq $SELECTOR_USER_CODE64, REG_CS(%rsp)
    movq                  %rcx, REG_RIP(%rsp)
    movq                    $0, REG_ERROR_CODE(%rsp)
    movq                 $0x00, REG_INT_VECTOR(%rsp)

    movq %r15, REG_R15(%rsp)
    movq %r14, REG_R14(%rsp)
    movq %r13, REG_R13(%rsp)
    movq %r12, REG_R12(%rsp)
    movq %r11, REG_R11(%rsp)
    movq %r10, REG_R10(%rsp)
    movq  %r9, REG_R9(%rsp)
    movq  %r8, REG_R8(%rsp)
    movq %rdi, REG_RDI(%rsp)
    movq %rsi, REG_RSI(%rsp)
    movq %rbp, REG_RBP(%rsp)
    movq %rdx, REG_RDX(%rsp)
    movq %rcx, REG_RCX(%rsp)
    movq %rbx, REG_RBX(%rsp)
    movq   $0, REG_RAX(%rsp)

    xorq %rax, %rax
    movw  %gs, %ax
    movq %rax, REG_GS(%rsp)
    movw  %fs, %ax
    movq %rax, REG_FS(%rsp)
    movw  %es, %ax
    movq %rax, REG_ES(%rsp)
    movw  %ds, %ax
    movq %rax, REG_DS(%rsp)
    subq   $8, %rsp         // Pad (unused)

    leaq             8(%rsp), %rdi
    leaq syscall_entry(%rip), %rax
    callq *%rax
    addq $8, %rsp

    movq REG_RAX(%rsp), %rax
    movq REG_RBX(%rsp), %rbx
    movq REG_RCX(%rsp), %rcx
    movq REG_RDX(%rsp), %rdx
    movq REG_RBP(%rsp), %rbp
    movq REG_RSI(%rsp), %rsi
    movq REG_RDI(%rsp), %rdi
    movq  REG_R8(%rsp), %r8
    movq  REG_R9(%rsp), %r9
    movq REG_R10(%rsp), %r10
    movq REG_R11(%rsp), %r11
    movq REG_R12(%rsp), %r12
    movq REG_R13(%rsp), %r13
    movq REG_R14(%rsp), %r14
    movq REG_R15(%rsp), %r15

    movq REG_RSP(%rsp), %rsp

    sysretq