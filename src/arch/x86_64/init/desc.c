// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc/desc.h>
#include <asm/desc/gdt.h>
#include <asm/desc/idt.h>
#include <asm/desc/tss.h>

void init_desc(void)
{
    gdt_init();
    idt_init();
    tss_init();
    return;
}

void ap_init_desc(void)
{
    load_gdt();
    load_idt();
    tss_init();
}