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

extern SYSV_ABI void asm_syscall_entry(void);

void arch_syscall_init(void)
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

static void print_registers(pt_regs_t *regs)
{
    uint64_t ds = regs->ds;
    uint64_t es = regs->es;
    uint64_t fs = regs->fs;
    uint64_t gs = regs->gs;

    uint64_t rax = regs->rax;
    uint64_t rbx = regs->rbx;
    uint64_t rcx = regs->rcx;
    uint64_t rdx = regs->rdx;
    uint64_t rbp = regs->rbp;
    uint64_t rsi = regs->rsi;
    uint64_t rdi = regs->rdi;

    uint64_t r8  = regs->r8;
    uint64_t r9  = regs->r9;
    uint64_t r10 = regs->r10;
    uint64_t r11 = regs->r11;
    uint64_t r12 = regs->r12;
    uint64_t r13 = regs->r13;
    uint64_t r14 = regs->r14;
    uint64_t r15 = regs->r15;

    uint64_t rip    = regs->rip;
    uint64_t cs     = regs->cs;
    uint64_t rflags = regs->rflags;
    uint64_t rsp    = regs->rsp;
    uint64_t ss     = regs->ss;

    printk("Registers:\n");
    printk("RIP: " MSG_HIGHLIGHT("%04x:%016lx\n"), cs, rip);
    printk("RSP: " MSG_HIGHLIGHT("%04x:%016lx "), ss, rsp);
    printk("FLAGS: " MSG_HIGHLIGHT("%08x\n"), rflags);

    printk("RAX: " MSG_HIGHLIGHT("%016lx "), rax);
    printk("RBX: " MSG_HIGHLIGHT("%016lx "), rbx);
    printk("RCX: " MSG_HIGHLIGHT("%016lx\n"), rcx);

    printk("RDX: " MSG_HIGHLIGHT("%016lx "), rdx);
    printk("RSI: " MSG_HIGHLIGHT("%016lx "), rsi);
    printk("RDI: " MSG_HIGHLIGHT("%016lx\n"), rdi);

    printk("RBP: " MSG_HIGHLIGHT("%016lx "), rbp);
    printk("R08: " MSG_HIGHLIGHT("%016lx "), r8);
    printk("R09: " MSG_HIGHLIGHT("%016lx\n"), r9);

    printk("R10: " MSG_HIGHLIGHT("%016lx "), r10);
    printk("R11: " MSG_HIGHLIGHT("%016lx "), r11);
    printk("R12: " MSG_HIGHLIGHT("%016lx\n"), r12);

    printk("R13: " MSG_HIGHLIGHT("%016lx "), r13);
    printk("R14: " MSG_HIGHLIGHT("%016lx "), r14);
    printk("R15: " MSG_HIGHLIGHT("%016lx\n"), r15);

    printk("FS:  " MSG_HIGHLIGHT("%04x "), fs);
    printk("GS:  " MSG_HIGHLIGHT("%04x "), gs);
    printk("CS:  " MSG_HIGHLIGHT("%04x "), cs);
    printk("DS: " MSG_HIGHLIGHT("%04x "), ds);
    printk("ES: " MSG_HIGHLIGHT("%04x \n"), es);
    return;
}

void syscall_entry(pt_regs_t *regs);
void syscall_entry(pt_regs_t *regs)
{
    printk(MSG_INFO "syscall:\n");
    print_registers(regs);
    return;
}