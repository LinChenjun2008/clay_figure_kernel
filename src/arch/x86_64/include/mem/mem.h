// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __MEM_H__
#define __MEM_H__

#define IS_AVAILABLE_ADDRESS(ADDR)             \
    ((uint64_t)(ADDR) >= 0xffff800000000000 && \
     (uint64_t)(ADDR) <= 0xffff8000ffffffff)

PUBLIC void   mem_init(void);
PUBLIC size_t total_memory(void);

#endif