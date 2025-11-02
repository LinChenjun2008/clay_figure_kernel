// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#ifndef __MM_STRUCT_H__
#define __MM_STRUCT_H__

#include <sync/spinlock.h> // spinlock

typedef struct mm_block_s
{
    uintptr_t start;
    size_t    size;
} mm_block_t;

typedef struct mm_struct_s
{
    mm_block_t *blocks;
    uint64_t    total_blocks;
    uint64_t    using_blocks; // 有记录的block数量
    spinlock_t  lock;
} mm_struct_t;

PUBLIC void
mm_struct_init(mm_struct_t *mm, mm_block_t *blocks, uint64_t total_blocks);

PUBLIC status_t
mm_remove_range_sub(mm_struct_t *mm, uintptr_t start, size_t size);
PUBLIC status_t mm_remove_range(mm_struct_t *mm, uintptr_t start, size_t size);
PUBLIC status_t mm_add_range_sub(mm_struct_t *mm, uintptr_t start, size_t size);
PUBLIC status_t mm_add_range(mm_struct_t *table, uintptr_t start, size_t size);

PUBLIC status_t mm_alloc_sub(mm_struct_t *mm, size_t size, void *vaddr);
PUBLIC status_t mm_alloc(mm_struct_t *mm, size_t size, void *vaddr);

PUBLIC int mm_find(mm_struct_t *mm, uintptr_t addr);

PUBLIC int mm_traversal(
    mm_struct_t *mm,
    int (*function)(mm_block_t *, uint64_t),
    uint64_t arg
);
#endif