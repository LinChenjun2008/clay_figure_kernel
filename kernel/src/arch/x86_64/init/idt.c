// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc.h>
#include <asm/utils.h>

static struct gate_desc idt[256];

static void set_gatedesc(struct gate_desc *gd, void *func, int selector, int ar)
{
    gd->offset_low  = ((uint64_t)func) & 0x000000000000ffff;
    gd->selector    = selector;
    gd->ist         = 0;
    gd->attribute   = (ar & 0xff);
    gd->offset_mid  = (uint32_t)((((uint64_t)func) >> 16) & 0x000000000000ffff);
    gd->offset_high = (uint32_t)((((uint64_t)func) >> 32) & 0x00000000ffffffff);
    return;
}

#define INTR_HANDLER(ENTRY, NR, ERROR_CODE) \
    void ASMLINKAGE ENTRY(struct pt_regs *);
#include <asm/interrupt.h>
#undef INTR_HANDLER

static void idt_desc_init(void)
{
#define INTR_HANDLER(ENTRY, NR, ERROR_CODE) \
    set_gatedesc(&idt[NR], ENTRY, SELECTOR_KERNEL_CODE64, AR_IDT_DESC_DPL0);
#include <asm/interrupt.h>
#undef INTR_HANDLER
    return;
}

void idt_init(void)
{
    idt_desc_init();
    load_idt();
    return;
}

void load_idt(void)
{
    uint64_t idt_ptr[2];
    idt_ptr[0] = ((((uint64_t)idt)) << 16) | (sizeof(idt) - 1);
    idt_ptr[1] = ((((uint64_t)idt)) >> 48) & 0xffff;
    asm_lidt(&idt_ptr);
    return;
}