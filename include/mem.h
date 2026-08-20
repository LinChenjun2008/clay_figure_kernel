// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_H__
#define __MEM_H__

#define MIN_BLOCK_SIZE  64   //  64 Byte
#define MAX_BLOCK_SIZE  1024 //   1 KiB
#define MAX_BLOCK_TYPES 5

void mem_init(struct boot_info *boot_info);

// allocator.c
void mem_allocator_init(void);

void *kmalloc(size_t size, size_t alignment, size_t boundary);
void  kfree(void *addr);

#endif