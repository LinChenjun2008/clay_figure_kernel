// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_PAGE_H__
#define __MEM_PAGE_H__

#include <asm/page.h>

// page.c
void page_mgr_init(struct system_info *system_info);

void     page_struct_lock(size_t pfn);
void     page_struct_unlock(size_t pfn);
void     page_reference_inc_lock(size_t pfn);
uint32_t page_reference_dec_lock(size_t pfn);
uint32_t page_reference_read_lock(size_t pfn);
void     page_reference_inc(size_t pfn);
uint32_t page_reference_dec(size_t pfn);
uint32_t page_reference_read(size_t pfn);
void    *allocate_pages(size_t pages);
void    *allocate_a_page(void);
size_t   free_pages(void *addr, size_t pages);
size_t   free_a_page(void *addr);

#endif /* __MEM_PAGE_H__ */
