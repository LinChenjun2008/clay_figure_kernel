// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_STRUCT_H__
#define __MEM_STRUCT_H__

#include <lib/free_table.h>
#include <lib/linked_list.h>
#include <sync/atomic.h>

struct page
{
    uint64_t      pfn;
    struct atomic reference_count;
    uint64_t      flags;
    uint64_t      count;
};

struct mm
{
    struct free_table vmemmap;
    struct free_table pmemmap;
};

#endif /* __MEM_STRUCT_H__ */