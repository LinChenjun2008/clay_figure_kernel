// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#include <kernel/global.h>

#include <device/spinlock.h> // spinlock
#include <mem/mm_struct.h>   // allocate_table_t,allocate_table_entry_t
#include <std/string.h>      // memset

PUBLIC void
mm_struct_init(mm_struct_t *mm, mm_block_t *blocks, uint64_t total_blocks)
{
    mm->blocks       = blocks;
    mm->total_blocks = total_blocks;
    mm->using_blocks = 0;
    memset(blocks, 0, sizeof(mm_block_t) * total_blocks);
    init_spinlock(&mm->lock);
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

PUBLIC status_t
mm_remove_range_sub(mm_struct_t *mm, uintptr_t start, size_t size)
{
    uintptr_t end = start + size;
    uint64_t  i;

    status_t ret = K_ERROR;

    uintptr_t block_start = 0;
    uintptr_t block_end   = 0;

    // 遍历所有空闲块
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
    // 情况2：范围在块中间，需要分割
    if (start > block_start && end < block_end)
    {
        // 检查是否有空间存储新块
        if (mm->using_blocks >= mm->total_blocks)
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
    // 情况3：范围在块开头
    if (start == block_start)
    {
        mm->blocks[i].start = end;
        mm->blocks[i].size  = block_end - end;

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
    spinlock_lock(&mm->lock);
    status_t ret = mm_remove_range_sub(mm, start, size);
    spinlock_unlock(&mm->lock);
    return ret;
}

PUBLIC status_t mm_add_range_sub(mm_struct_t *mm, uintptr_t start, size_t size)
{
    uint64_t i;
    status_t ret = K_ERROR;
    for (i = 0; i < mm->using_blocks; i++)
    {
        // [i - 1].start < start < [i].start
        if (mm->blocks[i].start > start)
        {
            break;
        }
    }
    // i > 0: 前面存在block,尝试与前面的block合并
    if (i > 0)
    {
        if (mm->blocks[i - 1].start + mm->blocks[i - 1].size == start)
        {
            mm->blocks[i - 1].size += size;
            // 当前是最后一个,结束
            if (i == mm->using_blocks)
            {
                ret = K_SUCCESS;
                goto done;
            }

            // 尝试与block[i]合并
            if (start + size == mm->blocks[i].start)
            {
                mm->blocks[i - 1].size += mm->blocks[i].size;
                move_forward(mm, i);
                mm->using_blocks--;
            }
            ret = K_SUCCESS;
            goto done;
        }
    }
    // 不能和前面的合并
    if (i < mm->using_blocks)
    {
        // 与后面的合并
        if (start + size == mm->blocks[i].start)
        {
            mm->blocks[i].start = start;
            mm->blocks[i].size += size;
            ret = K_SUCCESS;
            goto done;
        }
    }
    // 无法合并,创建新block
    if (mm->using_blocks < mm->total_blocks)
    {
        move_backward(mm, i);
        mm->using_blocks++;
        mm->blocks[i].start = start;
        mm->blocks[i].size  = size;

        ret = K_SUCCESS;
        goto done;
    }
done:
    return ret;
}

PUBLIC status_t mm_add_range(mm_struct_t *mm, uintptr_t start, size_t size)
{
    status_t ret = K_ERROR;
    spinlock_lock(&mm->lock);
    ret = mm_add_range_sub(mm, start, size);
    spinlock_unlock(&mm->lock);
    return ret;
}

PUBLIC status_t mm_alloc(mm_struct_t *mm, size_t size, void *vaddr)
{
    uint64_t  i;
    uintptr_t start;
    status_t  ret = K_OUT_OF_RESOURCE;
    spinlock_lock(&mm->lock);
    for (i = 0; i < mm->using_blocks; i++)
    {
        if (mm->blocks[i].size >= size)
        {
            start = mm->blocks[i].start;
            mm_remove_range_sub(mm, start, size);
            *(uintptr_t *)vaddr = start;

            ret = K_SUCCESS;
            break;
        }
    }
    spinlock_unlock(&mm->lock);
    return ret;
}

PUBLIC int mm_find(mm_struct_t *mm, uintptr_t addr)
{
    uintptr_t block_start;
    uintptr_t block_end;

    int ret = 0;
    spinlock_lock(&mm->lock);
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
        }
    }
    spinlock_unlock(&mm->lock);
    return ret;
}

PUBLIC int mm_traversal(
    mm_struct_t *mm,
    int (*function)(mm_block_t *, uint64_t),
    uint64_t arg
)
{
    int ret = 0;
    spinlock_lock(&mm->lock);
    uint64_t i;
    for (i = 0; i < mm->using_blocks; i++)
    {
        ret = function(&mm->blocks[i], arg);
        if (ret != 0)
        {
            break;
        }
    }
    spinlock_unlock(&mm->lock);
    return ret;
}