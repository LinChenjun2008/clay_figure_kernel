// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/sync/spinlock.h>

#include <mm_struct.h>
#include <print.h>
#include <std/string.h>

static void      move_forward(mm_struct_t *mm, int index);
static void      move_backward(mm_struct_t *mm, int index);
static int       mm_combine_with_next(mm_struct_t *mm, int index);
static void      mm_combine(mm_struct_t *mm, int index);
static int       mm_add_lock(mm_struct_t *mm, uintptr_t start, size_t size);
static uintptr_t mm_allocate_lock(mm_struct_t *mm, size_t size);
static int       mm_remove_lock(mm_struct_t *mm, uintptr_t start, size_t size);

void init_mm_struct(mm_struct_t *mm, mm_block_t *blocks, int total_blocks)
{
    init_spinlock(&mm->lock);
    mm->blocks       = blocks;
    mm->total_blocks = total_blocks;
    mm->count        = 0;
    memset(blocks, 0, sizeof(*mm->blocks) * total_blocks);
    return;
}

/**
 * @brief 将index后方的项向前移动,index项将被index+1覆盖,count--
 * @param mm
 * @param index
 */
static void move_forward(mm_struct_t *mm, int index)
{
    int i = index;
    for (i = index; i < mm->count - 1; i++)
    {
        mm->blocks[i] = mm->blocks[i + 1];
    }
    mm->count--;
    return;
}

/**
 * @brief 将index后方的项向后移动,index项将空闲,count++
 * @param mm
 * @param index
 */
static void move_backward(mm_struct_t *mm, int index)
{
    int i;
    for (i = mm->count; i > index; i--)
    {
        mm->blocks[i] = mm->blocks[i - 1];
    }
    mm->count++;
    return;
}

/**
 * @brief 与下一项合并
 * @param mm
 * @param index
 * @return 1: 发生合并 0: 未合并
 */
static int mm_combine_with_next(mm_struct_t *mm, int index)
{

    uintptr_t start;
    size_t    size;

    int i = index;
    if (i < mm->count - 1)
    {
        start = mm->blocks[i].start;
        size  = mm->blocks[i].size;
        if (start + size == mm->blocks[i + 1].start)
        {
            mm->blocks[i].size += mm->blocks[i + 1].size;
            move_forward(mm, i + 1);
            return 1;
        }
    }
    return 0;
}

static void mm_combine(mm_struct_t *mm, int index)
{
    int i = index;
    mm_combine_with_next(mm, i);
    if (i > 0)
    {
        mm_combine_with_next(mm, i - 1);
    }
    return;
}

static int mm_add_lock(mm_struct_t *mm, uintptr_t start, size_t size)
{
    int i;
    for (i = 0; i < mm->count; i++)
    {
        if (start < mm->blocks[i].start)
        {
            break;
        }
    }
    if (mm->count >= mm->total_blocks)
    {
        return -1;
    }
    move_backward(mm, i);
    mm->blocks[i].start = start;
    mm->blocks[i].size  = size;
    mm_combine(mm, i);
    return 0;
}

int mm_add(mm_struct_t *mm, uintptr_t start, size_t size)
{
    int ret = 0;

    spin_lock(&mm->lock);
    ret = mm_add_lock(mm, start, size);
    spin_unlock(&mm->lock);

    return ret;
}

static uintptr_t mm_allocate_lock(mm_struct_t *mm, size_t size)
{
    uintptr_t start = -1UL;

    int i;
    for (i = 0; i < mm->count; i++)
    {
        if (mm->blocks[i].size >= size)
        {
            start = mm->blocks[i].start;
            mm_remove_lock(mm, start, size);
            break;
        }
    }
    return start;
}

uintptr_t mm_allocate(mm_struct_t *mm, size_t size)
{
    uintptr_t start = -1UL;

    spin_lock(&mm->lock);
    start = mm_allocate_lock(mm, size);
    spin_unlock(&mm->lock);

    return start;
}

static int mm_remove_lock(mm_struct_t *mm, uintptr_t start, size_t size)
{

    uintptr_t end = start + size;

    uintptr_t block_start = 0;
    uintptr_t block_end   = 0;

    int i;
    for (i = 0; i < mm->count; i++)
    {
        block_start = mm->blocks[i].start;
        block_end   = block_start + mm->blocks[i].size;

        if (start >= block_start && end <= block_end)
        {
            break;
        }
    }

    if (i == mm->count)
    {
        return -1;
    }

    // 匹配整个块
    if (start == block_start && end == block_end)
    {
        move_forward(mm, i);
        return 0;
    }

    // 在开头
    if (start == block_start)
    {
        mm->blocks[i].start = end;
        mm->blocks[i].size  = block_end - end;
        return 0;
    }

    // 在中间,分割块
    if (start > block_start && end < block_end)
    {
        if (mm->count + 1 >= mm->total_blocks)
        {
            return -2;
        }

        // 调整当前块为前半部分
        mm->blocks[i].size = start - block_start;

        // 插入新块（后半部分）
        move_backward(mm, i + 1);
        mm->blocks[i + 1].start = end;
        mm->blocks[i + 1].size  = block_end - end;
        return 0;
    }

    // 在结尾
    if (end == block_end)
    {
        mm->blocks[i].size = start - block_start;
        return 0;
    }
    return 0;
}

int mm_remove(mm_struct_t *mm, uintptr_t start, size_t size)
{
    int ret = 0;

    spin_lock(&mm->lock);
    ret = mm_remove_lock(mm, start, size);
    spin_unlock(&mm->lock);

    return ret;
}