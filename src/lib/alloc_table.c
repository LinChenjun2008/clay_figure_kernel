/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

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

PUBLIC status_t allocate_units(
    allocate_table_t *table,
    uint64_t          number_of_units,
    uint64_t         *index
)
{
    if (index == NULL)
    {
        return K_INVAILD_PARAM;
    }
    uint64_t i;
    uint64_t j;
    for (i = 0; i < table->number_of_entries; i++)
    {
        if (table->entries[i].number_of_units >= number_of_units)
        {
            j = table->entries[i].index;
            table->entries[i].index += number_of_units;
            table->entries[i].number_of_units -= number_of_units;
            if (table->entries[i].number_of_units == 0)
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

PUBLIC void
free_units(allocate_table_t *table, uint64_t index, uint64_t number_of_units)
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
        if (table->entries[i - 1].index +
                table->entries[i - 1].number_of_units ==
            index)
        {
            table->entries[i - 1].number_of_units += number_of_units;
            if (i < table->frees)
            {
                if (index + number_of_units == table->entries[i].index)
                {
                    table->entries[i - 1].number_of_units +=
                        table->entries[i].number_of_units;
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
        if (index + number_of_units == table->entries[i].index)
        {
            table->entries[i].index = index;
            table->entries[i].number_of_units += number_of_units;
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
        table->entries[i].index           = index;
        table->entries[i].number_of_units = number_of_units;
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
        free_units += table->entries[i].number_of_units;
    }
    return free_units;
}