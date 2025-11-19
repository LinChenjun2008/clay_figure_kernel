// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 Lin Chenjun
 */

#include <kernel/global.h>

#include <mem/mm_struct.h> // allocate_table_t,allocate_table_entry_t
#include <std/string.h>    // memset
#include <sync/spinlock.h> // spinlock

PUBLIC void
mm_struct_init(mm_struct_t *mm, mm_block_t *blocks, uint64_t total_blocks)
{
    mm->blocks       = blocks;
    mm->total_blocks = total_blocks;
    mm->using_blocks = 0;
    memset(blocks, 0, sizeof(*mm->blocks) * total_blocks);
    init_spin(&mm->lock);
    return;
}

PRIVATE void move_forward(mm_struct_t *mm, uint64_t index)
{
    uint64_t i = index;
    for (i = index; i < mm->using_blocks - 1; i++)
    {
        mm->blocks[i] = mm->blocks[i + 1];
    }
    return;
}

PRIVATE void move_backward(mm_struct_t *mm, uint64_t index)
{
    uint64_t i;
    for (i = mm->using_blocks; i > index; i--)
    {
        mm->blocks[i] = mm->blocks[i - 1];
    }
    return;
}

PRIVATE status_t
mm_remove_range_without_spin(mm_struct_t *mm, uintptr_t start, size_t size)
{
    uintptr_t end = start + size;

    status_t ret = K_ERROR;

    uintptr_t block_start = 0;
    uintptr_t block_end   = 0;

    // 遍历所有空闲块
    uint64_t i;
    for (i = 0; i < mm->using_blocks; i++)
    {
        block_start = mm->blocks[i].start;
        block_end   = block_start + mm->blocks[i].size;

        // 检查是否包含目标范围
        if (start >= block_start && end <= block_end)
        {
            break;
        }
    }

    // 未找到包含目标范围的块
    // (实际上只有可能取等)
    if (i >= mm->using_blocks)
    {
        ret = K_NOT_FOUND;
        goto done;
    }


    // 情况1：范围匹配整个块
    if (start == block_start && end == block_end)
    {
        move_forward(mm, i);
        mm->using_blocks--;
        ret = K_SUCCESS;
        goto done;
    }
    // 情况2：范围在块开头
    if (start == block_start)
    {
        mm->blocks[i].start = end;
        mm->blocks[i].size  = block_end - end;

        ret = K_SUCCESS;
        goto done;
    }
    // 情况3：范围在块中间，需要分割
    if (start > block_start && end < block_end)
    {
        // 检查是否有空间存储新块
        if (mm->using_blocks + 1 >= mm->total_blocks)
        {
            ret = K_OUT_OF_RESOURCE; // 块数组已满
            goto done;
        }

        // 调整当前块为前半部分
        mm->blocks[i].size = start - block_start;

        // 插入新块（后半部分）
        move_backward(mm, i + 1);
        mm->blocks[i + 1].start = end;
        mm->blocks[i + 1].size  = block_end - end;
        mm->using_blocks++;
        ret = K_SUCCESS;
        goto done;
    }
    // 情况4：范围在块末尾
    if (end == block_end)
    {
        mm->blocks[i].size = start - block_start;

        ret = K_SUCCESS;
        goto done;
    }
done:
    return ret;
}

PUBLIC status_t mm_remove_range(mm_struct_t *mm, uintptr_t start, size_t size)
{
    spin_lock(&mm->lock);
    status_t ret = mm_remove_range_without_spin(mm, start, size);
    spin_unlock(&mm->lock);
    return ret;
}

PRIVATE void mm_combine(mm_struct_t *mm)
{
    uintptr_t start;
    size_t    size;
    uintptr_t next_start;
    size_t    next_size;
    uint64_t  i = 0;
    while (i < mm->using_blocks - 1)
    {
        start      = mm->blocks[i].start;
        size       = mm->blocks[i].size;
        next_start = mm->blocks[i + 1].start;
        next_size  = mm->blocks[i + 1].size;
        if (start + size == next_start)
        {
            mm->blocks[i].size += next_size;
            move_forward(mm, i + 1);
            mm->using_blocks--;
        }
        else
        {
            i++;
        }
    }
    return;
}

PUBLIC status_t mm_add_range(mm_struct_t *mm, uintptr_t start, size_t size)
{
    spin_lock(&mm->lock);
    uint64_t i;
    status_t ret = K_SUCCESS;
    for (i = 0; i < mm->using_blocks; i++)
    {
        // [i - 1].start < start < [i].start
        if (mm->blocks[i].start > start)
        {
            break;
        }
    }
    // 检查是否有空间存储新块
    if (mm->using_blocks >= mm->total_blocks)
    {
        ret = K_OUT_OF_RESOURCE; // 块数组已满
        goto done;
    }
    move_backward(mm, i);
    mm->using_blocks++;
    mm->blocks[i].start = start;
    mm->blocks[i].size  = size;
    mm_combine(mm);
done:
    spin_unlock(&mm->lock);
    return ret;
}

PUBLIC status_t mm_alloc(mm_struct_t *mm, size_t size, void *addr)
{
    spin_lock(&mm->lock);
    uintptr_t start;
    status_t  ret = K_OUT_OF_RESOURCE;
    uint64_t  i;
    for (i = 0; i < mm->using_blocks; i++)
    {
        if (mm->blocks[i].size >= size)
        {
            start = mm->blocks[i].start;
            mm_remove_range_without_spin(mm, start, size);
            *(uintptr_t *)addr = start;

            ret = K_SUCCESS;
            break;
        }
    }
    spin_unlock(&mm->lock);

    return ret;
}

PUBLIC int mm_find(mm_struct_t *mm, uintptr_t addr)
{
    uintptr_t block_start;
    uintptr_t block_end;

    int ret = 0;
    spin_lock(&mm->lock);
    // 遍历所有空闲块
    uint64_t i;
    for (i = 0; i < mm->using_blocks; i++)
    {
        block_start = mm->blocks[i].start;
        block_end   = block_start + mm->blocks[i].size;
        // 检查是否包含目标范围
        if (addr >= block_start && addr < block_end)
        {
            ret = 1;
            break;
        }
    }
    spin_unlock(&mm->lock);
    return ret;
}

PUBLIC int mm_traversal(
    mm_struct_t *mm,
    int (*function)(mm_block_t *, uint64_t),
    uint64_t arg
)
{
    int ret = 0;
    spin_lock(&mm->lock);
    uint64_t i;
    for (i = 0; i < mm->using_blocks; i++)
    {
        ret = function(&mm->blocks[i], arg);
        if (ret != 0)
        {
            break;
        }
    }
    spin_unlock(&mm->lock);
    return ret;
}