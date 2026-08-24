// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_ALLOCATOR_H__
#define __MEM_ALLOCATOR_H__

void mem_allocator_init(void);

void *kmalloc(size_t size, size_t alignment, size_t boundary);
void  kfree(void *addr);

#endif /* __MEM_ALLOCATOR_H__ */
