// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_H__
#define __MEM_H__

#include <lib/free_table.h>
#include <lib/linked_list.h>

#define MIN_BLOCK_SIZE  64   //  64 Byte
#define MAX_BLOCK_SIZE  1024 //   1 KiB
#define MAX_BLOCK_TYPES 5

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

void mem_init(struct boot_info *boot_info);

// allocator.c
void mem_allocator_init(void);

void *kmalloc(size_t size, size_t alignment, size_t boundary);
void  kfree(void *addr);

#endif