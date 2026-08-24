// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <lib/free_table.h>
#include <mem.h>

void init_free_table(struct free_table *free_table, int step)
{
    if (step <= 0)
    {
        step = 1;
    }
    free_table->table    = NULL;
    free_table->capacity = 0;
    free_table->step     = step;
    free_table->free     = 0;
    return;
}

void destroy_free_table(struct free_table *free_table)
{
    kfree(free_table->table);
    free_table->table    = NULL;
    free_table->capacity = 0;
    free_table->step     = 0;
    free_table->free     = 0;
    return;
}

// 扩大table容量,每次以step为单位动态分配
static int grow(struct free_table *free_table)
{
    int new_capacity = free_table->capacity + free_table->step;

    struct free_block *new_table =
        kmalloc(new_capacity * sizeof(struct free_block), 0, 0);
    if (new_table == NULL)
    {
        return -1;
    }

    int i;
    for (i = 0; i < free_table->free; i++)
    {
        new_table[i] = free_table->table[i];
    }
    kfree(free_table->table);

    free_table->table    = new_table;
    free_table->capacity = new_capacity;
    return 0;
}

// 当空闲的块(未使用容量)达到step * 2时,扣除step个块的空间
static void shrink(struct free_table *free_table)
{
    if (free_table->capacity <= free_table->step)
    {
        return;
    }
    if (free_table->capacity - free_table->free < free_table->step * 2)
    {
        return;
    }
    struct free_block *new_table    = NULL;
    int                new_capacity = free_table->capacity - free_table->step;

    new_table = kmalloc(new_capacity * sizeof(struct free_block), 0, 0);
    if (new_table == NULL)
    {
        return;
    }

    int i;
    for (i = 0; i < free_table->free; i++)
    {
        new_table[i] = free_table->table[i];
    }
    kfree(free_table->table);

    free_table->table    = new_table;
    free_table->capacity = new_capacity;
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
    if (free_table->free >= free_table->capacity)
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

int free_table_add(struct free_table *free_table, uintptr_t start, size_t size)
{
    if (free_table->free >= free_table->capacity)
    {
        if (grow(free_table) != 0 || free_table->free >= free_table->capacity)
        {
            return -1;
        }
    }
    int i = find_insert_position(free_table, start);
    move_backward(free_table, i);
    free_table->table[i].start = start;
    free_table->table[i].size  = size;
    combine(free_table, i);
    shrink(free_table);
    return 0;
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

int free_table_remove(
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
        shrink(free_table);
        return 0;
    }

    if (start == block_start)
    {
        free_table->table[i].start = end;
        free_table->table[i].size -= size;
        shrink(free_table);
        return 0;
    }

    if (start > block_start && end < block_end)
    {
        if (free_table->free >= free_table->capacity)
        {
            if (grow(free_table) != 0 ||
                free_table->free >= free_table->capacity)
            {
                return -2;
            }
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
        shrink(free_table);
        return 0;
    }
    return -3;
}

uintptr_t free_table_allocate(struct free_table *free_table, size_t size)
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
        status = free_table_remove(free_table, start, size);
        if (status == 0)
        {
            ret = start;
            break;
        }
    }
    return ret;
}

int free_table_find(struct free_table *free_table, uintptr_t start)
{
    uintptr_t block_start;
    uintptr_t block_end;

    int i;
    for (i = 0; i < free_table->free; i++)
    {
        block_start = free_table->table[i].start;
        block_end   = block_start + free_table->table[i].size;
        if (block_start <= start && start < block_end)
        {
            return 1;
        }
    }
    return 0;
}