// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MM_STRUCT_H__
#define __MM_STRUCT_H__

#include <asm/sync/spinlock.h>

struct mm_block
{
    uintptr_t start;
    size_t    size;
};

struct mm
{
    struct spinlock  lock;
    struct mm_block *blocks;
    int              count;
    int              total_blocks;
};

void init_mm_struct(
    struct mm       *mm_struct,
    struct mm_block *blocks,
    int              total_blocks
);

int       mm_add(struct mm *mm_struct, uintptr_t start, size_t size);
uintptr_t mm_allocate(struct mm *mm_struct, size_t size);
int       mm_remove(struct mm *mm_struct, uintptr_t start, size_t size);

#endif /* __MM_STRUCT_H__ */