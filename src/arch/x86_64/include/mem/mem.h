/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#ifndef __MEM_H__
#define __MEM_H__


PUBLIC void     mem_init(void);
PUBLIC size_t   total_memory(void);
PUBLIC uint32_t total_pages(void);
PUBLIC uint32_t total_free_pages(void);

#endif