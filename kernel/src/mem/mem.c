// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <mem.h>
#include <print.h>

void mem_init(struct boot_info *boot_info)
{
    printk("mem_init: page management initializing...\n");
    pg_allocator_init(boot_info);
    printk("mem_init: memory management initializing...\n");
    mem_allocator_init();
    return;
}