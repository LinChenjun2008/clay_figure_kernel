// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __MEM_H__
#define __MEM_H__


PUBLIC void     mem_init(void);
PUBLIC size_t   total_memory(void);
PUBLIC uint32_t total_pages(void);
PUBLIC uint32_t total_free_pages(void);

#endif