// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#include <kernel/global.h>

#include <lib/alloc_table.h> // allocate_table_t,allocate_table_entry_t

PUBLIC void allocate_table_init(
    allocate_table_t       *table,
    allocate_table_entry_t *entries,
    uint64_t                number_of_entries
)
{
    table->number_of_entries = number_of_entries;
    table->frees             = 0;
    table->entries           = entries;
    return;
}

PUBLIC status_t
allocate_units(allocate_table_t *table, uint64_t count, uint64_t *index)
{
    if (index == NULL)
    {
        return K_INVALID_PARAM;
    }
    uint64_t i;
    uint64_t j;
    for (i = 0; i < table->frees; i++)
    {
        if (table->entries[i].count >= count)
        {
            j = table->entries[i].index;
            table->entries[i].index += count;
            table->entries[i].count -= count;
            if (table->entries[i].count == 0)
            {
                table->frees--;
                while (i < table->frees)
                {
                    table->entries[i] = table->entries[i + 1];
                    i++;
                }
            }
            *index = j;
            return K_SUCCESS;
        }
    }
    return K_OUT_OF_RESOURCE;
}

PUBLIC void free_units(allocate_table_t *table, uint64_t index, uint64_t count)
{
    uint64_t i, j;
    for (i = 0; i < table->frees; i++)
    {
        if (table->entries[i].index > index)
        {
            break;
        }
    }
    if (i > 0)
    {
        if (table->entries[i - 1].index + table->entries[i - 1].count == index)
        {
            table->entries[i - 1].count += count;
            if (i < table->frees)
            {
                if (index + count == table->entries[i].index)
                {
                    table->entries[i - 1].count += table->entries[i].count;
                    table->frees--;
                    while (i < table->frees)
                    {
                        table->entries[i] = table->entries[i + 1];
                        i++;
                    }
                }
            }
            return;
        }
    }
    if (i < table->frees)
    {
        if (index + count == table->entries[i].index)
        {
            table->entries[i].index = index;
            table->entries[i].count += count;
            return;
        }
    }
    if (table->frees < table->number_of_entries)
    {
        for (j = table->frees; j > i; j--)
        {
            table->entries[j] = table->entries[j - 1];
        }
        table->frees++;
        table->entries[i].index = index;
        table->entries[i].count = count;
        return;
    }
    return;
}

PUBLIC uint64_t total_free_units(allocate_table_t *table)
{
    uint64_t i;
    uint64_t free_units = 0;
    for (i = 0; i < table->frees; i++)
    {
        free_units += table->entries[i].count;
    }
    return free_units;
}