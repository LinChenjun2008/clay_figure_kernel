// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <mem/allocator.h> // previous for mem_alloctor_init
#include <mem/mem.h>       // previous for mem_init
#include <mem/page.h>      // previous for mem_page_init
PUBLIC void mem_init(void)
{
    mem_page_init();
    mem_allocator_init();
    return;
}