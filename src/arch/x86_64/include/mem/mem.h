/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __MEM_H__
#define __MEM_H__

PUBLIC void     mem_init(void);
PUBLIC size_t   total_memory(void);
PUBLIC uint32_t total_pages(void);
PUBLIC uint32_t total_free_pages(void);

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

PUBLIC void mem_allocator_init(void);

/**
 * @brief 在内存池中分配size大小的内存块
 * @param size 内存块大小
 * @note size <= MAX_ALLOCATE_MEMORY_SIZE
 * @param alignment 对齐大小,为0则不对齐
 * @param boundary 边界限制,为0则不限制
 * @param addr 如果成功,addr指针处存储了分配到的物理地址
 * @return 成功将返回K_SUCCESS,失败则返回错误码
 */
PUBLIC status_t
pmalloc(size_t size, size_t alignment, size_t boundary, void *addr);

/**
 * @brief 在内存池中释放addr处的内存块.
 * @param addr 将释放的地址
 * @note 此处的addr应为pmalloc所返回的地址.
 */
PUBLIC void pfree(void *addr);

#endif