// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#ifndef __ALLOC_TABLE_H__
#define __ALLOC_TABLE_H__

typedef struct allocate_table_entry_s
{
    uint64_t index;
    uint64_t count;
} allocate_table_entry_t;

typedef struct allocate_table_s
{
    uint64_t                number_of_entries;
    uint64_t                frees;
    allocate_table_entry_t *entries;
} allocate_table_t;

PUBLIC void allocate_table_init(
    allocate_table_t       *table,
    allocate_table_entry_t *entries,
    uint64_t                number_of_entries
);

PUBLIC status_t
allocate_units(allocate_table_t *table, uint64_t count, uint64_t *index);

PUBLIC void free_units(allocate_table_t *table, uint64_t index, uint64_t count);

PUBLIC uint64_t total_free_units(allocate_table_t *table);

#endif