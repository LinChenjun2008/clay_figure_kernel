// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <lib/free_table.h>

void init_free_table(struct free_table *free_table, void *table, int total)
{
    init_spinlock(&free_table->lock);
    free_table->table = table;
    free_table->total = total;
    free_table->free  = 0;
    return;
}

// 从index开始的块在表中向前移动,并减小free值
static void move_forward(struct free_table *free_table, int index)
{
    if (index <= 0 || index > free_table->free)
    {
        return;
    }
    int i;
    for (i = index; i < free_table->free; i++)
    {
        free_table->table[i - 1] = free_table->table[i];
    }
    free_table->table[i - 1].start = 0;
    free_table->table[i - 1].size  = 0;
    free_table->free--;
    return;
}

// 将index开始的块向后移动,free++
static void move_backward(struct free_table *free_table, int index)
{
    if (free_table->free >= free_table->total)
    {
        return;
    }
    int i;
    for (i = free_table->free; i > index; i--)
    {
        free_table->table[i] = free_table->table[i - 1];
    }
    free_table->free++;
    return;
}

// 尝试与下一块合并
static void combine_with_next(struct free_table *free_table, int index)
{
    uintptr_t start;
    size_t    size;
    int       i = index;

    // 最后一个
    if (i >= free_table->free - 1)
    {
        return;
    }
    start = free_table->table[i].start;
    size  = free_table->table[i].size;
    if (start + size == free_table->table[i + 1].start)
    {
        free_table->table[i].size += free_table->table[i + 1].size;
        move_forward(free_table, i + 2);
    }
    return;
}

// 尝试将index与前后两块合并
static void combine(struct free_table *free_table, int index)
{
    combine_with_next(free_table, index);
    if (index > 0)
    {
        combine_with_next(free_table, index - 1);
    }
    return;
}

static int find_insert_position(struct free_table *free_table, uintptr_t start)
{
    int left = 0, right = free_table->free;
    int mid;
    while (left < right)
    {
        mid = (left + right) / 2;
        if (free_table->table[mid].start < start)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }
    return left;
}

static int
free_table_add_lock(struct free_table *free_table, uintptr_t start, size_t size)
{
    if (free_table->free >= free_table->total)
    {
        return -1;
    }
    int i = find_insert_position(free_table, start);
    move_backward(free_table, i);
    free_table->table[i].start = start;
    free_table->table[i].size  = size;
    combine(free_table, i);
    return 0;
}

int free_table_add(struct free_table *free_table, uintptr_t start, size_t size)
{
    int ret = 0;

    spin_lock(&free_table->lock);
    ret = free_table_add_lock(free_table, start, size);
    spin_unlock(&free_table->lock);
    return ret;
}

static int
find_contain_block(struct free_table *free_table, uintptr_t start, size_t size)
{
    int pos  = 0;
    int left = 0, right = free_table->free;
    int mid;
    while (left < right)
    {
        mid = (left + right) / 2;
        if (free_table->table[mid].start <= start)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }
    pos = left - 1;
    if (pos < 0)
    {
        return -1;
    }
    uintptr_t end         = start + size;
    uintptr_t block_start = free_table->table[pos].start;
    uintptr_t block_end   = block_start + free_table->table[pos].size;
    if (block_start <= start && end <= block_end)
    {
        return pos;
    }
    return -1;
}

static int free_table_remove_lock(
    struct free_table *free_table,
    uintptr_t          start,
    size_t             size
)
{
    int i = find_contain_block(free_table, start, size);
    if (i < 0)
    {
        return -1;
    }

    uintptr_t end         = start + size;
    uintptr_t block_start = free_table->table[i].start;
    uintptr_t block_end   = block_start + free_table->table[i].size;

    if (start == block_start && end == block_end)
    {
        move_forward(free_table, i + 1);
        return 0;
    }

    if (start == block_start)
    {
        free_table->table[i].start = end;
        free_table->table[i].size -= size;
        return 0;
    }

    if (start > block_start && end < block_end)
    {
        if (free_table->free >= free_table->total)
        {
            return -2;
        }
        free_table->table[i].size = start - block_start;

        move_backward(free_table, i + 1);
        free_table->table[i + 1].start = end;
        free_table->table[i + 1].size  = block_end - end;
        return 0;
    }
    if (end == block_end)
    {
        free_table->table[i].size -= size;
        return 0;
    }
    return -3;
}

int free_table_remove(
    struct free_table *free_table,
    uintptr_t          start,
    size_t             size
)
{
    int ret = 0;
    spin_lock(&free_table->lock);
    ret = free_table_remove_lock(free_table, start, size);
    spin_unlock(&free_table->lock);

    return ret;
}

static uintptr_t
free_table_allocate_lock(struct free_table *free_table, size_t size)
{
    uintptr_t ret = -1UL;
    uintptr_t start;
    int       status = 0;
    int       i;
    for (i = 0; i < free_table->free; i++)
    {
        if (free_table->table[i].size < size)
        {
            continue;
        }
        start  = free_table->table[i].start;
        status = free_table_remove_lock(free_table, start, size);
        if (status == 0)
        {
            ret = start;
            break;
        }
    }
    return ret;
}

uintptr_t free_table_allocate(struct free_table *free_table, size_t size)
{
    uintptr_t ret;

    spin_lock(&free_table->lock);
    ret = free_table_allocate_lock(free_table, size);
    spin_unlock(&free_table->lock);

    return ret;
}