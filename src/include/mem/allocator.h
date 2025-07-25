// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

#define MIN_ALLOCATE_MEMORY_SIZE     2048   //   2 KiB
#define MAX_ALLOCATE_MEMORY_SIZE     262144 // 256 KiB
#define NUMBER_OF_MEMORY_BLOCK_TYPES 8

PUBLIC void mem_allocator_init(void);

/**
 * @brief 在内存池中分配size大小的内存块
 * @param size 内存块大小
 * @note size <= MAX_ALLOCATE_MEMORY_SIZE
 * @param alignment 对齐大小,为0则不对齐
 * @param boundary 边界限制,为0则不限制
 * @param addr 如果成功,addr指针处存储了分配到的虚拟地址
 * @return 成功将返回K_SUCCESS,失败则返回错误码
 */
PUBLIC status_t
kmalloc(size_t size, size_t alignment, size_t boundary, void *addr);

/**
 * @brief 在内存池中释放addr处的内存块.
 * @param addr 将释放的地址
 * @note 此处的addr应为kmalloc所返回的地址,
 *       或者是可通过PHYS_TO_VIRT转换为kmalloc所返回的地址的虚拟地址.
 */
PUBLIC void kfree(void *addr);

#endif