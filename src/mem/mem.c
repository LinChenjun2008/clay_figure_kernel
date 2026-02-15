// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/mem/page.h>

#include <mem.h>
#include <mem/allocator.h>
#include <print.h>

void mem_init(boot_info_t *boot_info)
{
    printk("mem_init: page management initializing...\n");
    page_init(boot_info);
    printk("mem_init: memory allocator initializing...\n");
    mem_allocator_init();
    return;
}