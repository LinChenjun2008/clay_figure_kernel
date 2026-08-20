// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <mem.h>
#include <print.h>

void mem_init(struct system_info *system_info)
{
    printk("mem_init: page management initializing...\n");
    page_mgr_init(system_info);
    printk("mem_init: memory management initializing...\n");
    mem_allocator_init();
    return;
}