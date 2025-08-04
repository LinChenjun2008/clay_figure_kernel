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
    return;
}

PRIVATE void move_forward(vmm_struct_t *vmm, uint64_t index)
{
    uint64_t i = index;
    for (i = index; i < vmm->frees; i++) vmm->blocks[i] = vmm->blocks[i + 1];
}

PRIVATE void move_backward(vmm_struct_t *vmm, uint64_t index)
{
    uint64_t i;
    for (i = vmm->frees; i > index; i--) vmm->blocks[i] = vmm->blocks[i - 1];
}

PUBLIC status_t vmm_alloc(vmm_struct_t *vmm, size_t size, void *vaddr)
{
    uint64_t  i;
    uintptr_t ret;
    for (i = 0; i < vmm->frees; i++)
    {
        if (vmm->blocks[i].size >= size)
        {
            ret = vmm->blocks[i].start;
            vmm->blocks[i].start += size;
            vmm->blocks[i].size -= size;
            if (vmm->blocks[i].size == 0)
            {
                vmm->frees--;
                move_forward(vmm, i);
            }
            *(uintptr_t *)vaddr = ret;
            return K_SUCCESS;
        }
    }
    return K_OUT_OF_RESOURCE;
}

PUBLIC void vmm_free(vmm_struct_t *vmm, uintptr_t start, size_t size)
{
    uint64_t i;
    for (i = 0; i < vmm->frees; i++)
    {
        if (vmm->blocks[i].start > start)
        {
            // [i - 1].start < start < [i].start
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
            if (i == vmm->frees)
            {
                return;
            }
            // 尝试与block[i]合并
            if (start + size == vmm->blocks[i].start)
            {
                vmm->blocks[i - 1].size += vmm->blocks[i].size;
                vmm->frees--;
                move_forward(vmm, i);
            }
            return;
        }
    }
    // 不能和前面的合并
    if (i < vmm->frees)
    {
        // 与后面的合并
        if (start + size == vmm->blocks[i].start)
        {
            vmm->blocks[i].start = start;
            vmm->blocks[i].size += size;
            return;
        }
    }
    // 无法合并,创建新block
    if (vmm->frees < vmm->total_blocks)
    {
        move_backward(vmm, i);
        vmm->frees++;
        vmm->blocks[i].start = start;
        vmm->blocks[i].size  = size;
        return;
    }
    /// TODO: 处理分配失败
    return;
}
