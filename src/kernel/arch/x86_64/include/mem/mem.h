#ifndef __MEM_H__
#define __MEM_H__

PUBLIC void mem_init();
PUBLIC status_t alloc_physical_page(IN(uint64_t number_of_pages),OUT(void *addr));
PUBLIC void free_physical_page(void *addr,uint64_t number_of_pages);

PUBLIC uint64_t* pml4t_entry(void *pml4t,void *vaddr);
PUBLIC uint64_t* pdpt_entry(void *pml4t,void *vaddr);
PUBLIC uint64_t* pdt_entry(void *pml4t,void *vaddr);
PUBLIC void* to_physical_address(void *pml4t,void *vaddr);

PUBLIC void page_map(uint64_t *pml4t,void *paddr,void *vaddr);
PUBLIC void page_unmap(uint64_t *pml4t,void *vaddr);

PUBLIC void mem_alloctor_init();
PUBLIC status_t pmalloc(IN(size_t size),OUT(void *addr));
PUBLIC void pfree(void *addr);

#endif