// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#ifndef __PAGE_H__
#define __PAGE_H__

#define PT_SIZE 0x1000
#define PG_SIZE 0x200000

// Present
#define PG_P (1 << 0)

// Read/Write
#define PG_RW_R (0 << 1)
#define PG_RW_W (1 << 1)

// User/Supervisor
#define PG_US_S (0 << 2)
#define PG_US_U (1 << 2)

// Page Chace Disable
#define PG_PCD           (1 << 4)
#define PG_SIZE_2M       (1 << 7)
#define PG_DEFAULT_FLAGS (PG_US_U | PG_RW_W | PG_P | PG_SIZE_2M)

#define ADDR_PML4T_INDEX_SHIFT 39
#define ADDR_PML4T_INDEX_MASK  0x1ff
#define ADDR_PDPT_INDEX_SHIFT  30
#define ADDR_PDPT_INDEX_MASK   0x1ff
#define ADDR_PDT_INDEX_SHIFT   21
#define ADDR_PDT_INDEX_MASK    0x1ff
#define ADDR_OFFSET_SHIFT      0
#define ADDR_OFFSET_MASK       0x1fffff


#define KERNEL_VMA_BASE  0xffff800000000000
#define KERNEL_TEXT_BASE 0xffffffff80000000

#define KERNEL_PAGE_DIR_TABLE_POS 0x0000000000510000

#define PADDR_AVAILABLE(ADDR) (ADDR <= 0x00007fffffffffff)

#define KADDR_P2V(ADDR) ((void *)((addr_t)(ADDR) + KERNEL_VMA_BASE))
#define KADDR_V2P(ADDR) ((void *)((addr_t)(ADDR) - KERNEL_VMA_BASE))

#define PAGE_BITMAP_BYTES_LEN 2048

#ifndef __ASM_INCLUDE__

PUBLIC void   mem_page_init(void);
PUBLIC size_t get_total_free_pages(void);

/**
 * @brief 分配number_of_pages个连续的大小为PG_SIZE的物理页
 * @param number_of_pages 要分配的页数
 * @param addr 如果成功,addr指针处存储了分配到的物理页基地址
 * @return 成功将返回K_SUCCESS,失败返回对应的错误码
 */
PUBLIC status_t alloc_physical_page(uint64_t number_of_pages, void *addr);

/**
 * @brief 在task_init阶段分配number_of_pages个连续的大小为PG_SIZE的物理页
 * @note 本函数不会操作自旋锁.
 * @param number_of_pages 要分配的页数
 * @param addr 如果成功,addr指针处存储了分配到的物理页基地址
 * @return 成功将返回K_SUCCESS,失败返回对应的错误码
 */
PUBLIC status_t init_alloc_physical_page(uint64_t number_of_pages, void *addr);

/**
 * @brief 释放从addr地址开始,number_of_pages个大小为PG_SZIE的物理页
 * @param addr 物理页基址
 * @param number_of_pages 要释放的页数
 * @note addr必须是PG_SIZE对齐的
 */
PUBLIC void free_physical_page(void *addr, uint64_t number_of_pages);

PUBLIC uint64_t *pml4t_entry(void *pml4t, void *vaddr);
PUBLIC uint64_t *pdpt_entry(void *pml4t, void *vaddr);
PUBLIC uint64_t *pdt_entry(void *pml4t, void *vaddr);

/**
 * @brief 将页表pml4t中的虚拟地址vaddr转换为对应的物理地址
 * @param pml4t 页表地址
 * @param vaddr 地址
 */
PUBLIC void *to_physical_address(void *pml4t, void *vaddr);

/**
 * @brief 在页表中将虚拟地址vaddr映射到物理地址paddr处
 * @param pml4t 页表地址
 * @param paddr 物理地址
 * @param vaddr 虚拟地址
 * @param flags 页属性
 */
PUBLIC void page_map(uint64_t *pml4t, void *paddr, void *vaddr);

/**
 * @brief 解除虚拟地址vaddr在页表中的映射
 * @param pml4t 页表地址
 * @param vaddr 虚拟地址
 */
PUBLIC void page_unmap(uint64_t *pml4t, void *vaddr);

/**
 * @brief 修改页属性
 * @param pml4t 页表地址
 * @param vaddr 虚拟地址
 * @param flags 页属性
 */
PUBLIC void set_page_flags(uint64_t *pml4t, void *vaddr, uint64_t flags);
#endif

#endif