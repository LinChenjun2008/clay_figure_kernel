/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU Lesser General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 宽通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 宽通用公共许可证，了解详情。

你应该随程序获得一份 GNU 宽通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#ifndef __MEM_H__
#define __MEM_H__

PUBLIC void mem_init();
PUBLIC size_t total_memory();
PUBLIC uint32_t total_pages();
PUBLIC uint32_t total_free_pages();

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

PUBLIC void mem_allocator_init();

/**
 * 在内存池中分配size大小的内存块.
 * 其中 size <= MAX_ALLOCATE_MEMORY_SIZE.
 * 如果成功,addr指针处存储了分配到的物理地址.
 * 返回的地址按 (1 << n) 字节对齐,满足 size <= (1 << n),n为整数.
 */
PUBLIC status_t pmalloc(size_t size,void *addr);

/**
 * 在内存池中归还addr处的内存块.
 * 此处的addr应为pmalloc所返回的地址.
 */
PUBLIC void pfree(void *addr);

#endif