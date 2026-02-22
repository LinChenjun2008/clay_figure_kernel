// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/sync/spinlock.h>

#include <mm_struct.h>
#include <print.h>
#include <std/string.h>

static void move_forward(struct mm *mm_struct, int index);
static void move_backward(struct mm *mm_struct, int index);
static int  mm_combine_with_next(struct mm *mm_struct, int index);
static void mm_combine(struct mm *mm_struct, int index);
static int  mm_add_lock(struct mm *mm_struct, uintptr_t start, size_t size);
static uintptr_t mm_allocate_lock(struct mm *mm_struct, size_t size);
static int mm_remove_lock(struct mm *mm_struct, uintptr_t start, size_t size);

void init_mm_struct(
    struct mm       *mm_struct,
    struct mm_block *blocks,
    int              total_blocks
)
{
    init_spinlock(&mm_struct->lock);
    mm_struct->blocks       = blocks;
    mm_struct->total_blocks = total_blocks;
    mm_struct->count        = 0;
    memset(blocks, 0, sizeof(*mm_struct->blocks) * total_blocks);
    return;
}

/**
 * @brief 将index后方的项向前移动,index项将被index+1覆盖,count--
 * @param mm_struct
 * @param index
 */
static void move_forward(struct mm *mm_struct, int index)
{
    int i = index;
    for (i = index; i < mm_struct->count - 1; i++)
    {
        mm_struct->blocks[i] = mm_struct->blocks[i + 1];
    }
    mm_struct->count--;
    return;
}

/**
 * @brief 将index后方的项向后移动,index项将空闲,count++
 * @param mm_struct
 * @param index
 */
static void move_backward(struct mm *mm_struct, int index)
{
    int i;
    for (i = mm_struct->count; i > index; i--)
    {
        mm_struct->blocks[i] = mm_struct->blocks[i - 1];
    }
    mm_struct->count++;
    return;
}

/**
 * @brief 与下一项合并
 * @param mm_struct
 * @param index
 * @return 1: 发生合并 0: 未合并
 */
static int mm_combine_with_next(struct mm *mm_struct, int index)
{

    uintptr_t start;
    size_t    size;

    int i = index;
    if (i < mm_struct->count - 1)
    {
        start = mm_struct->blocks[i].start;
        size  = mm_struct->blocks[i].size;
        if (start + size == mm_struct->blocks[i + 1].start)
        {
            mm_struct->blocks[i].size += mm_struct->blocks[i + 1].size;
            move_forward(mm_struct, i + 1);
            return 1;
        }
    }
    return 0;
}

static void mm_combine(struct mm *mm_struct, int index)
{
    int i = index;
    mm_combine_with_next(mm_struct, i);
    if (i > 0)
    {
        mm_combine_with_next(mm_struct, i - 1);
    }
    return;
}

static int mm_add_lock(struct mm *mm_struct, uintptr_t start, size_t size)
{
    int i;
    for (i = 0; i < mm_struct->count; i++)
    {
        if (start < mm_struct->blocks[i].start)
        {
            break;
        }
    }
    if (mm_struct->count >= mm_struct->total_blocks)
    {
        return -1;
    }
    move_backward(mm_struct, i);
    mm_struct->blocks[i].start = start;
    mm_struct->blocks[i].size  = size;
    mm_combine(mm_struct, i);
    return 0;
}

int mm_add(struct mm *mm_struct, uintptr_t start, size_t size)
{
    int ret = 0;

    spin_lock(&mm_struct->lock);
    ret = mm_add_lock(mm_struct, start, size);
    spin_unlock(&mm_struct->lock);

    return ret;
}

static uintptr_t mm_allocate_lock(struct mm *mm_struct, size_t size)
{
    uintptr_t start = -1UL;

    int i;
    for (i = 0; i < mm_struct->count; i++)
    {
        if (mm_struct->blocks[i].size >= size)
        {
            start = mm_struct->blocks[i].start;
            mm_remove_lock(mm_struct, start, size);
            break;
        }
    }
    return start;
}

uintptr_t mm_allocate(struct mm *mm_struct, size_t size)
{
    uintptr_t start = -1UL;

    spin_lock(&mm_struct->lock);
    start = mm_allocate_lock(mm_struct, size);
    spin_unlock(&mm_struct->lock);

    return start;
}

static int mm_remove_lock(struct mm *mm_struct, uintptr_t start, size_t size)
{

    uintptr_t end = start + size;

    uintptr_t block_start = 0;
    uintptr_t block_end   = 0;

    int i;
    for (i = 0; i < mm_struct->count; i++)
    {
        block_start = mm_struct->blocks[i].start;
        block_end   = block_start + mm_struct->blocks[i].size;

        if (start >= block_start && end <= block_end)
        {
            break;
        }
    }

    if (i == mm_struct->count)
    {
        return -1;
    }

    // 匹配整个块
    if (start == block_start && end == block_end)
    {
        move_forward(mm_struct, i);
        return 0;
    }

    // 在开头
    if (start == block_start)
    {
        mm_struct->blocks[i].start = end;
        mm_struct->blocks[i].size  = block_end - end;
        return 0;
    }

    // 在中间,分割块
    if (start > block_start && end < block_end)
    {
        if (mm_struct->count + 1 >= mm_struct->total_blocks)
        {
            return -2;
        }

        // 调整当前块为前半部分
        mm_struct->blocks[i].size = start - block_start;

        // 插入新块（后半部分）
        move_backward(mm_struct, i + 1);
        mm_struct->blocks[i + 1].start = end;
        mm_struct->blocks[i + 1].size  = block_end - end;
        return 0;
    }

    // 在结尾
    if (end == block_end)
    {
        mm_struct->blocks[i].size = start - block_start;
        return 0;
    }
    return 0;
}

int mm_remove(struct mm *mm_struct, uintptr_t start, size_t size)
{
    int ret = 0;

    spin_lock(&mm_struct->lock);
    ret = mm_remove_lock(mm_struct, start, size);
    spin_unlock(&mm_struct->lock);

    return ret;
}