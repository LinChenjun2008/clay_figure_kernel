// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_STRUCT_H__
#define __MEM_STRUCT_H__

#include <lib/bitmap.h>
#include <lib/free_table.h>
#include <lib/linked_list.h>
#include <sync/atomic.h>

struct page_allocator
{
    struct free_table pg_map;
    struct atomic    *pg_ref_count;
};

enum mm_type
{
    MM_TYPE_FREE = 1,
    MM_TYPE_RESERVED,
    MM_TYPE_UNUSEABLE,
    MAX_MM_TYPE,
};

struct mem_group
{
    size_t          block_size;
    uint32_t        total_free;
    struct list     free_block_list;
    struct spinlock lock;
};

struct mem_cache
{
    struct mem_group *group;
    size_t            number_of_blocks;
    size_t            count;
};

struct mem_block
{
    uint64_t         magic;
    struct list_node node;
};

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