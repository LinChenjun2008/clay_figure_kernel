/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU Lesser General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 宽通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 宽通用公共许可证，了解详情。

你应该随程序获得一份 GNU 宽通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#ifndef __ALLOC_TABLE_H__
#define __ALLOC_TABLE_H__

typedef struct allocate_table_entry_s
{
    uint64_t index;
    uint64_t number_of_units;
} allocate_table_entry_t;

typedef struct allocate_table_s
{
    uint64_t                number_of_entries;
    uint64_t                frees;
    allocate_table_entry_t *entries;
} allocate_table_t;

PUBLIC void allocate_table_init(
    allocate_table_t *table,
    allocate_table_entry_t *entries,
    uint64_t number_of_entries);

PUBLIC status_t allocate_units(
    allocate_table_t *table,
    uint64_t number_of_units,
    uint64_t *index);

PUBLIC void free_units(
    allocate_table_t *table,
    uint64_t index,
    uint64_t number_of_units);

PUBLIC uint64_t total_free_units(allocate_table_t *table);

#endif