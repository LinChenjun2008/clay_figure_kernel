// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc.h>
#include <asm/ptrace.h>
#include <asm/syscall.h>
#include <asm/utils.h>
#include <asm/x86.h>

#include <print.h>
#include <syscall.h>

extern ASMLINKAGE void asm_syscall_entry(void);

extern void *syscall_table[NR_CONT];

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
        printk(MSG_WARN "syscall %#llx not supported.\n", func);
        regs->rax = -1;
        return;
    }
    if (syscall_table[func] == NULL)
    {
        printk(MSG_WARN "syscall %#llx not supported.\n", func);
        regs->rax = -2;
        return;
    }

    int (*sys_func)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
    sys_func  = syscall_table[func];
    int ret   = sys_func(regs->rsi, regs->rdx, regs->r10, regs->r8, regs->r9);
    regs->rax = ret;
    return;
}