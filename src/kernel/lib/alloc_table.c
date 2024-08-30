/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.
Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/global.h>
#include <lib/alloc_table.h> // allocate_table_t,allocate_table_entry_t

PUBLIC void allocate_table_init(
    allocate_table_t *table,
    allocate_table_entry_t *entries,
    uint64_t number_of_entries)
{
    table->number_of_entries = number_of_entries;
    table->frees             = 0;
    table->entries           = entries;
    return;
}

PUBLIC status_t allocate_units(
    allocate_table_t *table,uint64_t number_of_units,
    uint64_t *index)
{
    if (index == NULL)
    {
        return K_ERROR;
    }
    uint64_t i;
    uint64_t j;
    for (i = 0;i < table->number_of_entries;i++)
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
    return K_ERROR;
}

PUBLIC void free_units(
    allocate_table_t *table,
    uint64_t index,
    uint64_t number_of_units)
{
    uint64_t i,j;
    for (i = 0;i < table->frees;i++)
    {
        if (table->entries[i].index > index)
        {
            break;
        }
    }
    if (i > 0)
    {
        if (table->entries[i - 1].index + table->entries[i - 1].number_of_units
            == index)
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
        for (j = table->frees;j > i;j--)
        {
            table->entries[j] = table->entries[j - 1];
        }
        table->frees++;
        table->entries[i].index = index;
        table->entries[i].number_of_units = number_of_units;
        return;
    }
    return;
}

PUBLIC uint64_t total_free_units(allocate_table_t *table)
{
    uint64_t i;
    uint64_t free_units = 0;
    for (i = 0;i < table->frees;i++)
    {
        free_units += table->entries[i].number_of_units;
    }
    return free_units;
}