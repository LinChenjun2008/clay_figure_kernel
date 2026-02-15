// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_ALLOCATOR_H__
#define __MEM_ALLOCATOR_H__

#define MIN_ALLOCATE_MEMORY_SIZE     64   //  64 Byte
#define MAX_ALLOCATE_MEMORY_SIZE     1024 //   1 KiB
#define NUMBER_OF_MEMORY_BLOCK_TYPES 5

void mem_allocator_init(void);

/**
 * @brief 内核通用内存分配函数
 * @param size 要分配的大小
 * @param alignment 对齐
 * @param boundary 边界
 * @param addr 分配结果存储在该指针处
 * @return 非零值表示失败
 */
int kmalloc(size_t size, size_t alignment, size_t boundary, void **addr);

/**
 * @brief 释放内存
 * @param addr 该指针处存储要释放的地址值.释放成功,指针都将被清零
 */
void kfree(void **addr);

#endif /* __MEM_ALLOCATOR_H__ */