// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __LIB_FREE_TABLE_H__
#define __LIB_FREE_TABLE_H__

#include <asm/sync/spinlock.h>

struct free_block
{
    uintptr_t start;
    size_t    size;
};

struct free_table
{
    struct spinlock    lock;
    struct free_block *table;
    int                total;
    int                free;
};

void init_free_table(struct free_table *free_table, void *table, int total);
int free_table_add(struct free_table *free_table, uintptr_t start, size_t size);
int free_table_remove(
    struct free_table *free_table,
    uintptr_t          start,
    size_t             size
);
uintptr_t free_table_allocate(struct free_table *free_table, size_t size);

#endif /* __LIB_FREE_TABLE_H__ */