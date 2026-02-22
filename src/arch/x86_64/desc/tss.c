// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc/desc.h> // AR_XXX,SELECTOR_XXX
#include <asm/desc/gdt.h>
#include <asm/desc/tss.h>
#include <asm/drivers/apic.h> // apic_id
#include <asm/mem/page.h>     // PG_SIZE
#include <asm/utils.h>

#include <std/string.h> // memset,memcpy

extern struct segmdesc gdt_table[8192];
static struct tss64    tss_table[256];

static void init_tss(uint8_t cpu_id)
{
    uint32_t tss_size = sizeof(tss_table[0]);
    memset(&tss_table[cpu_id], 0, tss_size);
    tss_table[cpu_id].io_map = tss_size << 16;
    uint64_t tss_base_lo     = ((uint64_t)&tss_table[cpu_id]) & 0xffffffff;
    uint64_t tss_base_hi = (((uint64_t)&tss_table[cpu_id]) >> 32) & 0xffffffff;

    struct segmdesc *gdt_entry = &gdt_table[5 + cpu_id * 2];

    *gdt_entry = make_segmdesc(tss_base_lo, tss_size - 1, AR_TSS64);
    memcpy(gdt_entry + 1, &tss_base_hi, 8);

    return;
}

static void load_tss(uint8_t cpu_id)
{
    asm_ltr(SELECTOR_TSS(cpu_id));
    return;
}

void update_tss_rsp0(struct task *task)
{
    uint64_t kstack_base = task->kstack_base + task->kstack_pages * PG_SIZE;
    tss_table[task->cpu->id].rsp0 = kstack_base;
    return;
}

void tss_init(void)
{
    init_tss(apic_id());
    load_tss(apic_id());
    return;
}
