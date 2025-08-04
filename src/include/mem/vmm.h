// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#ifndef __VMM_H__
#define __VMM_H__

typedef struct vmm_block_s
{
    uintptr_t start;
    size_t    size;
} vmm_block_t;

typedef struct vmm_struct_s
{
    uint64_t     total_blocks;
    uint64_t     frees; // vmm有记录的block数量
    vmm_block_t *blocks;
} vmm_struct_t;

PUBLIC void
vmm_struct_init(vmm_struct_t *vmm, vmm_block_t *blocks, uint64_t total_blocks);

PUBLIC status_t vmm_alloc(vmm_struct_t *vmm, size_t size, void *vaddr);
PUBLIC void     vmm_free(vmm_struct_t *table, uintptr_t start, size_t size);

#endif