/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#include <kernel/global.h>

#include <mem/mem.h>  // previous for mem_init
#include <mem/page.h> // previous for mem_page_init

PUBLIC void mem_init(void)
{
    mem_page_init();
    return;
}