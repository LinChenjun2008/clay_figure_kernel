// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc/desc.h>
#include <asm/ptrace.h>
#include <asm/syscall.h>
#include <asm/utils.h>
#include <asm/x86.h>

#include <print.h>
#include <syscall.h>

extern ASMLINKAGE void asm_syscall_entry(void);

typedef int (*syscall_t)(struct pt_regs *);
extern syscall_t syscall_table[NR_CONT];

void arch_syscall_enable(void)
{
    uint64_t msr = 0;

    msr = rdmsr(IA32_EFER);
    msr |= IA32_EFER_SCE;
    wrmsr(IA32_EFER, msr);

    wrmsr(IA32_LSTAR, (uint64_t)asm_syscall_entry);

    // IA32_STAR[63:48] + 16 = user CS,IA32_STAR[63:48] + 8 = user SS
    msr = ((uint64_t)SELECTOR_KERNEL_CODE64 << 32);
    msr |= ((uint64_t)(SELECTOR_USER_CODE64 - 16) << 48);

    wrmsr(IA32_STAR, msr);
    wrmsr(IA32_FMASK, EFLAGS_IF_1);
    return;
}

void syscall_entry(struct pt_regs *regs);
void syscall_entry(struct pt_regs *regs)
{
    uint64_t func = regs->rdi;
    if (func >= NR_CONT)
    {
        printk(MSG_WARN "syscall %d not supported.\n", func);
        regs->rax = -1;
        return;
    }
    if (syscall_table[func] == NULL)
    {
        printk(MSG_WARN "syscall %d not supported.\n", func);
        regs->rax = -2;
        return;
    }

    // uint64_t arg1, arg2, arg3, arg4, arg5, ret;
    // arg1      = regs->rsi;
    // arg2      = regs->rdx;
    // arg3      = regs->rcx;
    // arg4      = regs->r8;
    // arg5      = regs->r9;

    int ret   = syscall_table[func](regs);
    regs->rax = ret;
    return;
}