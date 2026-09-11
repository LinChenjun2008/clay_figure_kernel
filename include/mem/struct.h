// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_STRUCT_H__
#define __MEM_STRUCT_H__

#include <lib/bitmap.h>
#include <lib/free_table.h>
#include <lib/linked_list.h>
#include <sync/spinlock.h>

#define MIN_BLOCK_SIZE  64   //  64 Byte
#define MAX_BLOCK_SIZE  2048 //   2 KiB
#define MAX_BLOCK_TYPES 6

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

#define MAX_ALLOCATE_PAGES 2048

#define PAGE_HEAD (1 << 0)

struct page
{
    int32_t         reference_count;
    uint16_t        flags;
    uint16_t        count;
    struct spinlock lock;
};

struct vm_struct
{
    struct free_table vm_table;
    struct free_table mapped;
    struct free_table unmapped;
    struct free_table copy_on_write;
};

struct page_struct
{
    struct list_node node;
    size_t           pfn;
    uintptr_t        virt;
};

struct pg_struct
{
    struct list list;
};

struct mm_struct
{
    struct vm_struct vm_map;
    struct pg_struct pg_map;
};

struct page_mgr
{
    struct spinlock   lock;
    struct bitmap     bitmap;  // 用于分配页的位图(1: free / 0: reserved)
    struct page      *pages;   // pages数组,用pfn为索引,负责页管理
    size_t            max_pfn; // 最大pfn
    struct mem_group *mem_groups;
};

#endif /* __MEM_STRUCT_H__ */