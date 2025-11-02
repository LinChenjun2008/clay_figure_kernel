// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 Lin Chenjun
 */

#include <kernel/global.h>

#include <mem/allocator.h>
#include <std/stdlib.h>

PUBLIC void *malloc(size_t size)
{
    void    *addr;
    status_t ret;
    ret = kmalloc(size, 0, 0, &addr);
    if (ret != K_SUCCESS)
    {
        return NULL;
    }
    return addr;
}

PUBLIC void free(void *addr)
{
    kfree(addr);
}
