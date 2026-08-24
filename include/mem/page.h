// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_PAGE_MGR_H__
#define __MEM_PAGE_MGR_H__

#include <asm/page.h>

// page.c
void page_mgr_init(struct system_info *system_info);

void     page_reference_inc(size_t pfn);
uint64_t page_reference_dec(size_t pfn);
void    *allocate_pages(size_t pages);
void     free_pages(void *addr, size_t pages);

#endif /* __MEM_PAGE_H__ */
