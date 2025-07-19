// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>
#include <kernel/symbols.h>

extern void              *kallsyms_address[] WEAK;
extern const char *const  kallsyms_symbols[] WEAK;
extern int kallsyms_count WEAK;

PUBLIC int is_available_symbol_address(void *addr)
{
    return addr >= (void *)&_kernel_start && addr <= (void *)&_kernel_end;
}

PUBLIC status_t get_symbol_index_by_addr(void *addr, int *index)
{
    int i;
    for (i = 0; i < kallsyms_count; i++)
    {
        if (addr >= kallsyms_address[i] && addr < kallsyms_address[i + 1])
        {
            *index = i;
            return K_SUCCESS;
        }
    }
    return K_ERROR;
}

PUBLIC const char *index_to_symbol(int index)
{
    if (index < kallsyms_count)
    {
        return kallsyms_symbols[index];
    }
    return "Invalid";
}

PUBLIC void *index_to_addr(int index)
{
    if (index < kallsyms_count)
    {
        return kallsyms_address[index];
    }
    return NULL;
}

PUBLIC const char *addr_to_symbol(void *addr)
{
    if (!is_available_symbol_address(addr))
    {
        return "Invalid";
    }
    int      i;
    status_t status = get_symbol_index_by_addr(addr, &i);
    if (ERROR(status))
    {
        return "Invalid";
    }
    return kallsyms_symbols[i];
}