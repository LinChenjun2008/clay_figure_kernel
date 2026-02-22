// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MM_STRUCT_H__
#define __MM_STRUCT_H__

#include <asm/sync/spinlock.h>

typedef struct mm_block_s
{
    uintptr_t start;
    size_t    size;
} mm_block_t;

typedef struct mm_struct_s
{
    struct spinlock lock;
    mm_block_t     *blocks;
    int             count;
    int             total_blocks;
} mm_struct_t;

void      init_mm_struct(mm_struct_t *mm, mm_block_t *blocks, int total_blocks);
int       mm_add(mm_struct_t *mm, uintptr_t start, size_t size);
uintptr_t mm_allocate(mm_struct_t *mm, size_t size);
int       mm_remove(mm_struct_t *mm, uintptr_t start, size_t size);

#endif /* __MM_STRUCT_H__ */