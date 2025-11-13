// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 Lin Chenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/cpu.h> // cpuid
#include <device/pic.h> // IRQ_CNT
#include <intr.h>
#include <io.h>             // get_flags
#include <kernel/symbols.h> // addr_to_symbol
#include <kernel/syscall.h> // sys_send_recv
#include <mem/mem.h>        // IS_AVAILABLE_ADDRESS
#include <service.h>        // MM_EXIT
#include <sync/spinlock.h>  // spinlock_t,spinlock_lock,spinlock_unlock
#include <task/task.h>      // task_struct_t,get_current_task

void (*irq_handler[IRQ_CNT])(intr_stack_t *);

PRIVATE const char *intr_name[20] = {
    "#DE", "#DB", "NMI", "#BP", "#OF", "#BR", "#UD", "#NM", "#DF", "CSO",
    "#TS", "#NP", "#SS", "#GP", "#PF", "RSV", "#MF", "#AC", "#MC", "#XF",
};

#pragma pack(1)
typedef struct
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  attribute;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} gate_desc_t;
#pragma pack()

PRIVATE gate_desc_t idt[IRQ_CNT];
PRIVATE spinlock_t  intr_lock;

PRIVATE void set_gatedesc(gate_desc_t *gd, void *func, int selector, int ar)
{
    gd->offset_low  = ((uint64_t)func) & 0x000000000000ffff;
    gd->selector    = selector;
    gd->ist         = 0;
    gd->attribute   = (ar & 0xff);
    gd->offset_mid  = (uint32_t)((((uint64_t)func) >> 16) & 0x000000000000ffff);
    gd->offset_high = (uint32_t)((((uint64_t)func) >> 32) & 0x00000000ffffffff);
    return;
}

PRIVATE void pr_debug_info(intr_stack_t *stack)
{
    uint64_t ds = stack->ds;
    uint64_t es = stack->es;
    uint64_t fs = stack->fs;
    uint64_t gs = stack->gs;

    uint64_t rax = stack->rax;
    uint64_t rbx = stack->rbx;
    uint64_t rcx = stack->rcx;
    uint64_t rdx = stack->rdx;
    uint64_t rbp = stack->rbp;
    uint64_t rsi = stack->rsi;
    uint64_t rdi = stack->rdi;

    uint64_t r8  = stack->r8;
    uint64_t r9  = stack->r9;
    uint64_t r10 = stack->r10;
    uint64_t r11 = stack->r11;
    uint64_t r12 = stack->r12;
    uint64_t r13 = stack->r13;
    uint64_t r14 = stack->r14;
    uint64_t r15 = stack->r15;

    uint64_t cs     = stack->cs;
    uint64_t rflags = stack->rflags;
    uint64_t rsp    = stack->rsp;
    uint64_t ss     = stack->ss;

    pr_msg("\n");
    pr_msg("CPUID: %d PID: %d\n", apic_id(), get_current_task()->pid);

    uintptr_t *rip;
    rip = *((uintptr_t **)rbp + 1);
    rbp = *(uintptr_t *)rbp;

    size_t    offset;
    uintptr_t sym_off;
    size_t    sym_len;
    int       sym_idx;
    status_t  status = get_symbol_index_by_addr(rip, &sym_idx);
    if (ERROR(status))
    {
        pr_msg("RIP: %04x:%016lx+%#x/%#x\n", cs, rip, 0, 0);
    }
    else
    {
        offset  = (uintptr_t)rip - BOOT_INFO->relocate_base;
        sym_off = offset - (uintptr_t)index_to_addr(sym_idx);
        sym_len =
            (size_t)index_to_addr(sym_idx + 1) - (size_t)index_to_addr(sym_idx);
        pr_msg(
            "RIP: %04x:%s+%#x/%#x\n",
            cs,
            index_to_symbol(sym_idx),
            sym_off,
            sym_len

        );
    }

    pr_msg("RSP: %04x:%016lx EFLAGS: %08x\n", ss, rsp, rflags);
    pr_msg("RAX: %016lx RBX: %016lx RCX: %016lx\n", rax, rbx, rcx);
    pr_msg("RDX: %016lx RSI: %016lx RDI: %016lx\n", rdx, rsi, rdi);
    pr_msg("PBP: %016lx R08: %016lx R09: %016lx\n", rbp, r8, r9);
    pr_msg("R10: %016lx R11: %016lx R12: %016lx\n", r10, r11, r12);
    pr_msg("R13: %016lx R14: %016lx R15: %016lx\n", r13, r14, r15);
    pr_msg(
        "FS:  %016lx(%04x) GS:%016lx(%04x) knlGS:%016lx\n",
        rdmsr(IA32_FS_BASE),
        fs,
        rdmsr(IA32_GS_BASE),
        gs,
        rdmsr(IA32_KERNEL_GS_BASE)
    );

    pr_msg("CS:  %04x DS: %04x ES: %04x CR0: %016lx\n", cs, ds, es, get_cr0());

    pr_msg(
        "CR2: %016lx CR3: %016lx CR4: %016lx\n", get_cr2(), get_cr3(), get_cr4()
    );

    // Backtrace

    pr_msg("Call trace:\n");
    while (1)
    {
        status = get_symbol_index_by_addr(rip, &sym_idx);
        if (ERROR(status))
        {
            break;
        }
        offset  = (uintptr_t)rip - BOOT_INFO->relocate_base;
        sym_off = offset - (uintptr_t)index_to_addr(sym_idx);
        sym_len =
            (size_t)index_to_addr(sym_idx + 1) - (size_t)index_to_addr(sym_idx);
        pr_msg(
            "    [<%p>] %s+%#x/%#x\n",
            rip,
            index_to_symbol(sym_idx),
            sym_off,
            sym_len

        );
        if (!IS_AVAILABLE_ADDRESS(rbp + 1))
        {
            break;
        }
        rip = *((uintptr_t **)rbp + 1);
        rbp = *(uintptr_t *)rbp;
    }
    pr_msg("---[end trace]---\n");
}

extern void ASMLINKAGE asm_panic();

PUBLIC void default_irq_handler(intr_stack_t *stack)
{
    int int_vector = stack->int_vector;
    if (int_vector == 0x27)
    {
        return;
    }
    spinlock_lock(&intr_lock);
    pr_msg("\n");
    pr_msg("INTR : 0x%x", int_vector);
    if (int_vector < 20)
    {
        pr_msg(": %s", intr_name[int_vector]);
    }
    pr_msg("\n");
    spinlock_unlock(&intr_lock);

    task_struct_t *running = get_current_task();
    if (running->page_dir != NULL)
    {
        proc_exit(-1);
    }
    PANIC(1, K_ERROR, "unknow interrupt.");
    while (1) continue;
}

PUBLIC void ASMLINKAGE do_irq(intr_stack_t *stack)
{
    int int_vector                  = stack->int_vector;
    void (*handler)(intr_stack_t *) = irq_handler[int_vector];
    handler != NULL ? handler(stack) : default_irq_handler(stack);
    return;
}

#define INTR_HANDLER(ENTRY, NR, ERROR_CODE) extern void ENTRY(intr_stack_t *);
#include <intr.h>
#undef INTR_HANDLER

PRIVATE void idt_desc_init(void)
{
#define INTR_HANDLER(ENTRY, NR, ERROR_CODE) \
    set_gatedesc(&idt[NR], ENTRY, SELECTOR_CODE64_K, AR_IDT_DESC_DPL0);
#include <intr.h>
#undef INTR_HANDLER
}

extern void ASMLINKAGE asm_lidt(void *idt_ptr);

PUBLIC void intr_init(void)
{
    idt_desc_init();
    int i;
    for (i = 0; i < IRQ_CNT; i++)
    {
        irq_handler[i] = default_irq_handler;
    }
    uint64_t idt_ptr[2];
    idt_ptr[0] = ((((uint64_t)idt)) << 16) | (sizeof(idt) - 1);
    idt_ptr[1] = ((((uint64_t)idt)) >> 48) & 0xffff;
    asm_lidt(&idt_ptr);
    init_spinlock(&intr_lock);

    irq_handler[0x82] = pr_debug_info;
    return;
}

PUBLIC void ap_intr_init(void)
{
    uint64_t idt_ptr[2];
    idt_ptr[0] = ((((uint64_t)idt)) << 16) | (sizeof(idt) - 1);
    idt_ptr[1] = ((((uint64_t)idt)) >> 48) & 0xffff;
    asm_lidt(&idt_ptr);
}

PUBLIC void register_handle(uint8_t int_vector, void (*handle)(intr_stack_t *))
{
    irq_handler[int_vector] = handle;
    return;
}

PUBLIC void unregister_handle(uint8_t int_vector)
{
    irq_handler[int_vector] = NULL;
    return;
}

PUBLIC intr_status_t intr_get_status(void)
{
    uint64_t flags;
    flags = get_flags();
    return ((flags & 0x00000200) ? INTR_ON : INTR_OFF);
}

PUBLIC intr_status_t intr_set_status(intr_status_t status)
{
    return (status == INTR_ON ? intr_enable() : intr_disable());
}

PUBLIC intr_status_t intr_enable(void)
{
    intr_status_t old_status;
    if (intr_get_status() == INTR_ON)
    {
        old_status = INTR_ON;
        return old_status;
    }
    else
    {
        old_status = INTR_OFF;
        io_sti();
        return old_status;
    }
}

PUBLIC intr_status_t intr_disable(void)
{
    intr_status_t old_status;
    if (intr_get_status() == INTR_ON)
    {
        old_status = INTR_ON;
        io_cli();
        return old_status;
    }
    else
    {
        old_status = INTR_OFF;
        return old_status;
    }
}