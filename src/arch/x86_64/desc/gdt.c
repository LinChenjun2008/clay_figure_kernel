// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc/desc.h>
#include <asm/desc/gdt.h>
#include <asm/utils.h>

struct segmdesc gdt_table[8192];

struct segmdesc make_segmdesc(uint32_t base, uint32_t limit, uint16_t access)
{
    struct segmdesc desc;
    desc.limit_low    = (limit & 0x0000ffff);
    desc.base_low     = (base & 0x0000ffff);
    desc.base_mid     = ((base & 0x00ff0000) >> 16);
    desc.access_right = (access & 0x00ff); // TYPE,S,DPL,P
    desc.limit_high =
        (((limit >> 16) & 0x0f) | ((access >> 8) & 0x00f0)); // AVL,L,D/B,G
    desc.base_high = ((base >> 24) & 0x00ff);
    return desc;
}

static void gdt_desc_init(void)
{
    gdt_table[0] = make_segmdesc(0, 0, 0);
    gdt_table[1] = make_segmdesc(0, 0, AR_CODE64);
    gdt_table[2] = make_segmdesc(0, 0, AR_DATA64);
    gdt_table[3] = make_segmdesc(0, 0, AR_DATA64_DPL3);
    gdt_table[4] = make_segmdesc(0, 0, AR_CODE64_DPL3);
    return;
}

void gdt_init(void)
{
    gdt_desc_init();
    load_gdt();
    return;
}

void load_gdt(void)
{
    uint64_t gdt_ptr[2];
    gdt_ptr[0] = (((uint64_t)gdt_table) << 16) | (sizeof(gdt_table) - 1);
    gdt_ptr[1] = (((uint64_t)gdt_table) >> 48) & 0xffff;
    asm_load_gdt(&gdt_ptr, SELECTOR_KERNEL_CODE64, SELECTOR_KERNEL_DATA64);
    return;
}
