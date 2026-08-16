// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/timer.h>
#include <asm/interrupt.h>
#include <asm/utils.h>
#include <asm/x86.h>

#include <print.h>
#include <task.h>

static int has_error_code[256] = {
#define INTR_HANDLER(ENTRY, NR, ERROR_CODE) !ERROR_CODE,
#include <asm/interrupt.h>
#undef INTR_HANDLER
};

static const char *intr_name[20] = {
    "Divide error",
    "Debuge exception",
    "NMI interrupt",
    "Breakpoint exception",
    "Overflow exception",
    "Bound Range exceeded exception",
    "Invailid opcode exception",
    "Device not available exception",
    "Double fault exception",
    "Coprocessor segment overrun",
    "Invailed TSS exception",
    "Segment not present",
    "Stack fault exception",
    "General protection exception",
    "Page-fault exception",
    "Reserved",
    "x87 FPU floationg-point error",
    "Alignment check exception",
    "Machine-check exception",
    "SIMD floating-point exception",
};

void (*irq_handler[INTR_CNT])(struct pt_regs *);

static void print_registers(struct pt_regs *regs)
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

    printk("FS:  " MSG_HIGHLIGHT("%016lx(%04x) "), rdmsr(IA32_FS_BASE), fs);
    printk("GS:" MSG_HIGHLIGHT("%016lx(%04x) "), rdmsr(IA32_GS_BASE), gs);
    printk("knlGS:" MSG_HIGHLIGHT("%016lx\n"), rdmsr(IA32_KERNEL_GS_BASE));

    printk("CS:  " MSG_HIGHLIGHT("%04x "), cs);
    printk("DS: " MSG_HIGHLIGHT("%04x "), ds);
    printk("ES: " MSG_HIGHLIGHT("%04x "), es);
    printk("CR0: " MSG_HIGHLIGHT("%016lx\n"), get_cr0());

    printk("CR2: " MSG_HIGHLIGHT("%016lx "), get_cr2());
    printk("CR3: " MSG_HIGHLIGHT("%016lx "), get_cr3());
    printk("CR4: " MSG_HIGHLIGHT("%016lx\n"), get_cr4());

    printk("current: %s.\n", get_current_task()->name);
    printk(MSG_ERR "CPU ID: %d.\n", get_current_cpu_id());
    return;
}

void general_handler(struct pt_regs *regs)
{
    if (regs->int_vector == 0x27)
    {
        return;
    }
    uint64_t nsecond = get_nano_time();
    uint64_t second  = nsecond / 1000000000;
    uint64_t usecond = (nsecond / 1000) % 1000000;

    uint64_t vector     = regs->int_vector;
    uint64_t error_code = regs->error_code;

    printk(MSG_ERR "Unexpected interrupt: " MSG_HIGHLIGHT("%#04x\n"), vector);
    printk(MSG_ERR "Time: " MSG_HIGHLIGHT("%u.%06u\n"), second, usecond);
    print_registers(regs);
    if (has_error_code[vector])
    {
        printk(MSG_ERR "Error code: " MSG_HIGHLIGHT("%08x\n"), error_code);
    }

    if (vector < 20)
    {
        printk(MSG_ERR MSG_HIGHLIGHT("%s") ".\n", intr_name[vector]);
    }

    while (1) io_hlt();
    return;
}

void ASMLINKAGE interrupt_handler(struct pt_regs *regs)
{
    int int_vector                    = regs->int_vector;
    void (*handler)(struct pt_regs *) = irq_handler[int_vector];
    handler != NULL ? handler(regs) : general_handler(regs);
    return;
}

static void debug_print(struct pt_regs *regs)
{
    print_registers(regs);
    while (1);
}

void intr_handler_init(void)
{
    int i;
    for (i = 0; i < INTR_CNT; i++)
    {
        register_handler(i, NULL);
    }
    register_handler(0xff, debug_print);
    return;
}

void register_handler(uint8_t vector, void *handler)
{
    irq_handler[vector] = handler;
    return;
}