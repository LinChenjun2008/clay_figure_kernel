/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __MEM_H__
#define __MEM_H__

PUBLIC void mem_init(void);
PUBLIC size_t total_memory(void);
PUBLIC uint32_t total_pages(void);
PUBLIC uint32_t total_free_pages(void);

/**
 * 分配number_of_pages个连续的大小为PG_SIZE的物理页
 * 如果成功,addr指针处存储了分配到的物理页基地址.
 */
PUBLIC status_t alloc_physical_page(uint64_t number_of_pages,void *addr);

/**
 * 在task_init阶段分配number_of_pages个连续的大小为PG_SIZE的物理页
 * 本函数不会操作自旋锁.
 * 如果成功,addr指针处存储了分配到的物理页基地址.
 */
PUBLIC status_t init_alloc_physical_page(uint64_t number_of_pages,void *addr);

/**
 * 归还从addr地址开始,number_of_pages个大小为PG_SZIE的物理页.
 * addr必须是PG_SIZE对齐的.
 */
PUBLIC void free_physical_page(void *addr,uint64_t number_of_pages);

PUBLIC uint64_t* pml4t_entry(void *pml4t,void *vaddr);
PUBLIC uint64_t* pdpt_entry(void *pml4t,void *vaddr);
PUBLIC uint64_t* pdt_entry(void *pml4t,void *vaddr);

/**
 * 将页表pml4t中的虚拟地址vaddr转换为对应的物理地址.
 */
PUBLIC void* to_physical_address(void *pml4t,void *vaddr);

/**
 * 在页表中将虚拟地址vaddr对应到物理地址paddr处.
 */
PUBLIC void page_map(uint64_t *pml4t,void *paddr,void *vaddr);

/**
 * 解除虚拟地址vaddr在页表中的映射.
 */
PUBLIC void page_unmap(uint64_t *pml4t,void *vaddr);

PUBLIC void mem_allocator_init(void);

/**
 * 在内存池中分配size大小的内存块.
 * 其中 size <= MAX_ALLOCATE_MEMORY_SIZE.
 * 如果成功,addr指针处存储了分配到的物理地址.
 * 返回的地址按 (1 << n) 字节对齐,满足 size <= (1 << n),n为整数.
 */
PUBLIC status_t pmalloc(size_t size,void *addr);
PUBLIC status_t pmalloc_align(size_t size,void *addr,size_t align);
/**
 * 在内存池中归还addr处的内存块.
 * 此处的addr应为pmalloc所返回的地址.
 */
PUBLIC void pfree(void *addr);

#endif