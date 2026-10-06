// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __LIB_FREE_TABLE_H__
#define __LIB_FREE_TABLE_H__

struct free_block
{
    uintptr_t start;
    size_t    size;
    int       flags;
};

struct free_table
{
    struct free_block *table;
    int                capacity;
    int                step;
    int                free;
};

void init_free_table(struct free_table *ft, int step);
void destroy_free_table(struct free_table *ft);

int ft_add(struct free_table *ft, uintptr_t start, size_t size, int flags);
int ft_remove(struct free_table *ft, uintptr_t start, size_t size, int flags);
intptr_t ft_allocate(struct free_table *ft, size_t size, int flags);
int      ft_find(struct free_table *ft, uintptr_t start, int flags);
int      ft_query_flags(struct free_table *ft, uintptr_t start, size_t size);
int      ft_set_flags(
    struct free_table *ft,
    uintptr_t          start,
    size_t             size,
    int                flags
);

int copy_free_table(struct free_table *dst, struct free_table *src);

#endif /* __LIB_FREE_TABLE_H__ */