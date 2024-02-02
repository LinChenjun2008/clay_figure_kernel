#ifndef __MEM_H__
#define __MEM_H__

PUBLIC void memory_init();
PUBLIC void *alloc_physical_page(uint64_t number_of_pages);
PUBLIC void free_physical_page(void *addr,uint64_t number_of_pages);

PUBLIC uint64_t *pml4t_entry(void *pml4t,void *vaddr);
PUBLIC uint64_t *pdpt_entry(void *pml4t,void *vaddr);
PUBLIC uint64_t *pdt_entry(void *pml4t,void *vaddr);
PUBLIC void *to_physical_address(void *pml4t,void *vaddr);

#endif