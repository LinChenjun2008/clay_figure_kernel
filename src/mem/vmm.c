// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#include <kernel/global.h>

#include <mem/vmm.h> // allocate_table_t,allocate_table_entry_t

PUBLIC void
vmm_struct_init(vmm_struct_t *vmm, vmm_block_t *blocks, uint64_t total_blocks)
{
    vmm->blocks       = blocks;
    vmm->total_blocks = total_blocks;
    vmm->using_blocks = 0;
    return;
}

PRIVATE void move_forward(vmm_struct_t *vmm, uint64_t index)
{
    uint64_t i = index;
    for (i = index; i < vmm->using_blocks; i++)
    {
        vmm->blocks[i] = vmm->blocks[i + 1];
    }
    return;
}

PRIVATE void move_backward(vmm_struct_t *vmm, uint64_t index)
{
    uint64_t i;
    for (i = vmm->using_blocks; i > index; i--)
    {
        vmm->blocks[i] = vmm->blocks[i - 1];
    }
    return;
}

PUBLIC status_t vmm_alloc(vmm_struct_t *vmm, size_t size, void *vaddr)
{
    uint64_t  i;
    uintptr_t start;
    for (i = 0; i < vmm->using_blocks; i++)
    {
        if (vmm->blocks[i].size >= size)
        {
            start = vmm->blocks[i].start;
            vmm_remove_range(vmm, start, size);
            *(uintptr_t *)vaddr = start;
            return K_SUCCESS;
        }
    }
    return K_OUT_OF_RESOURCE;
}

PUBLIC status_t vmm_add_range(vmm_struct_t *vmm, uintptr_t start, size_t size)
{
    uint64_t i;
    for (i = 0; i < vmm->using_blocks; i++)
    {
        // [i - 1].start < start < [i].start
        if (vmm->blocks[i].start > start)
        {
            break;
        }
    }

    // i > 0: 前面存在block,尝试与前面的block合并
    if (i > 0)
    {
        if (vmm->blocks[i - 1].start + vmm->blocks[i - 1].size == start)
        {
            vmm->blocks[i - 1].size += size;
            // 当前是最后一个,结束
            if (i == vmm->using_blocks)
            {
                return K_SUCCESS;
            }

            // 尝试与block[i]合并
            if (start + size == vmm->blocks[i].start)
            {
                vmm->blocks[i - 1].size += vmm->blocks[i].size;
                vmm->using_blocks--;
                move_forward(vmm, i);
            }
            return K_SUCCESS;
        }
    }
    // 不能和前面的合并
    if (i < vmm->using_blocks)
    {
        // 与后面的合并
        if (start + size == vmm->blocks[i].start)
        {
            vmm->blocks[i].start = start;
            vmm->blocks[i].size += size;
            return K_SUCCESS;
        }
    }
    // 无法合并,创建新block
    if (vmm->using_blocks < vmm->total_blocks)
    {
        move_backward(vmm, i);
        vmm->using_blocks++;
        vmm->blocks[i].start = start;
        vmm->blocks[i].size  = size;
        return K_SUCCESS;
    }
    return K_ERROR;
}

PUBLIC status_t
vmm_remove_range(vmm_struct_t *vmm, uintptr_t start, size_t size)
{
    uintptr_t end = start + size;
    uint64_t  i;

    uintptr_t block_start;
    uintptr_t block_end;
    // 遍历所有空闲块
    for (i = 0; i < vmm->using_blocks; i++)
    {
        block_start = vmm->blocks[i].start;
        block_end   = block_start + vmm->blocks[i].size;

        // 检查是否包含目标范围
        if (start >= block_start && end <= block_end)
        {
            break;
        }
    }

    // 未找到包含目标范围的块
    // (实际上只有可能取等)
    if (i >= vmm->using_blocks)
    {
        return K_NOT_FOUND;
    }


    // 情况1：范围匹配整个块
    if (start == block_start && end == block_end)
    {
        vmm->using_blocks--;
        move_forward(vmm, i);
        return K_SUCCESS;
    }
    // 情况2：范围在块中间，需要分割
    if (start > block_start && end < block_end)
    {
        // 检查是否有空间存储新块
        if (vmm->using_blocks >= vmm->total_blocks)
        {
            return K_OUT_OF_RESOURCE; // 块数组已满
        }

        // 调整当前块为前半部分
        vmm->blocks[i].size = start - block_start;

        // 插入新块（后半部分）
        move_backward(vmm, i + 1);
        vmm->blocks[i + 1].start = end;
        vmm->blocks[i + 1].size  = block_end - end;
        vmm->using_blocks++;
        return K_SUCCESS;
    }
    // 情况3：范围在块开头
    if (start == block_start)
    {
        vmm->blocks[i].start = end;
        vmm->blocks[i].size  = block_end - end;
        return K_SUCCESS;
    }
    // 情况4：范围在块末尾
    if (end == block_end)
    {
        vmm->blocks[i].size = start - block_start;
        return K_SUCCESS;
    }
    // 不应该到这里
    return K_ERROR;
}

PUBLIC int vmm_find(vmm_struct_t *vmm, uintptr_t addr)
{
    uintptr_t block_start;
    uintptr_t block_end;
    // 遍历所有空闲块
    uint64_t  i;
    for (i = 0; i < vmm->using_blocks; i++)
    {
        block_start = vmm->blocks[i].start;
        block_end   = block_start + vmm->blocks[i].size;
        // 检查是否包含目标范围
        if (addr >= block_start && addr <= block_end)
        {
            return 1;
        }
    }
    return 0;
}