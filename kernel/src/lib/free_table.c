// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <errno.h>
#include <lib/free_table.h>
#include <mem/allocator.h>
#include <panic.h>

void init_free_table(struct free_table *ft, int step)
{
    if (step <= 0)
    {
        step = 1;
    }
    ft->table    = NULL;
    ft->capacity = 0;
    ft->step     = step;
    ft->free     = 0;
    return;
}

void destroy_free_table(struct free_table *ft)
{
    kfree(ft->table);
    ft->table    = NULL;
    ft->capacity = 0;
    ft->step     = 0;
    ft->free     = 0;
    return;
}

// 扩大table容量,每次以step为单位动态分配
static int grow(struct free_table *ft)
{
    int new_capacity = ft->capacity + ft->step;

    struct free_block *new_table =
        kmalloc(new_capacity * sizeof(struct free_block), 0, 0);
    if (new_table == NULL)
    {
        return -ENOMEM;
    }

    int i;
    for (i = 0; i < ft->free; i++)
    {
        new_table[i] = ft->table[i];
    }
    kfree(ft->table);

    ft->table    = new_table;
    ft->capacity = new_capacity;
    return 0;
}

// 当空闲的块(未使用容量)达到step * 2时,扣除step个块的空间
static void shrink(struct free_table *ft)
{
    if (ft->capacity <= ft->step)
    {
        return;
    }
    if (ft->capacity - ft->free < ft->step * 2)
    {
        return;
    }
    struct free_block *new_table    = NULL;
    int                new_capacity = ft->capacity - ft->step;

    new_table = kmalloc(new_capacity * sizeof(struct free_block), 0, 0);
    if (new_table == NULL)
    {
        return;
    }

    int i;
    for (i = 0; i < ft->free; i++)
    {
        new_table[i] = ft->table[i];
    }
    kfree(ft->table);

    ft->table    = new_table;
    ft->capacity = new_capacity;
    return;
}

// 从index开始的块在表中向前移动,并减小free值
static void move_forward(struct free_table *ft, int index)
{
    if (index <= 0 || index > ft->free)
    {
        return;
    }
    int i;
    for (i = index; i < ft->free; i++)
    {
        ft->table[i - 1] = ft->table[i];
    }
    ft->table[i - 1].start = 0;
    ft->table[i - 1].size  = 0;
    ft->table[i - 1].flags = 0;
    ft->free--;
    return;
}

// 将index开始的块向后移动,free++
static void move_backward(struct free_table *ft, int index)
{
    if (ft->free >= ft->capacity)
    {
        return;
    }
    int i;
    for (i = ft->free; i > index; i--)
    {
        ft->table[i] = ft->table[i - 1];
    }
    ft->free++;
    return;
}

// 尝试与下一块合并
static void combine_with_next(struct free_table *ft, int index)
{
    int i = index;

    // 最后一个
    if (i >= ft->free - 1)
    {
        return;
    }
    uintptr_t start = ft->table[i].start;
    size_t    size  = ft->table[i].size;

    if (start + size != ft->table[i + 1].start)
    {
        return;
    }
    // 只有 flags 相同的区间才能合并
    if (ft->table[i].flags != ft->table[i + 1].flags)
    {
        return;
    }
    ft->table[i].size += ft->table[i + 1].size;
    move_forward(ft, i + 2);
    return;
}

// 尝试将index与前后两块合并
static void combine(struct free_table *ft, int index)
{
    combine_with_next(ft, index);
    if (index > 0)
    {
        combine_with_next(ft, index - 1);
    }
    return;
}

static int find_insert_position(struct free_table *ft, uintptr_t start)
{
    int left = 0, right = ft->free;
    int mid;
    while (left < right)
    {
        mid = (left + right) / 2;
        if (ft->table[mid].start < start)
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

// 向表中加入一个区间, flags 用于标记该区间
// flags < 0 视为非法; flags = 0 表示未标记
// 加入后相邻且 flags 相同的区间会被自动合并
// 失败(无法扩容)时返回 -ENOMEM 且不改动表
// 缩容失败不算失败(保持原容量, 等下一次调用再试)
int ft_add(struct free_table *ft, uintptr_t start, size_t size, int flags)
{
    if (flags < 0)
    {
        return -EINVAL;
    }
    if (size == 0 || start + size < start)
    {
        return -EINVAL;
    }
    if (ft->free >= ft->capacity)
    {
        if (grow(ft) < 0 || ft->free >= ft->capacity)
        {
            return -ENOMEM;
        }
    }
    int i = find_insert_position(ft, start);
    move_backward(ft, i);
    ft->table[i].start = start;
    ft->table[i].size  = size;
    ft->table[i].flags = flags;
    combine(ft, i);
    shrink(ft);
    return 0;
}

static int
find_contain_block(struct free_table *ft, uintptr_t start, size_t size)
{
    int pos  = 0;
    int left = 0, right = ft->free;
    int mid;
    while (left < right)
    {
        mid = (left + right) / 2;
        if (ft->table[mid].start <= start)
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
        return -ENOENT;
    }
    uintptr_t end         = start + size;
    uintptr_t block_start = ft->table[pos].start;
    uintptr_t block_end   = block_start + ft->table[pos].size;
    if (block_start <= start && end <= block_end)
    {
        return pos;
    }
    return -ENOENT;
}

// [start, start + size) 是否被相邻的区间完整覆盖
static int is_range_covered(struct free_table *ft, uintptr_t start, size_t size)
{
    uintptr_t end = start + size;
    int       i;
    for (i = 0; i < ft->free; i++)
    {
        if (start >= end)
        {
            return 1;
        }
        uintptr_t block_start = ft->table[i].start;
        uintptr_t block_end   = block_start + ft->table[i].size;
        if (block_start > start)
        {
            return 0; // 中间有空洞
        }
        if (start < block_end)
        {
            start = block_end;
        }
    }
    return start >= end;
}

// 若 addr 落在某个块内部, 就在该处把它切成两块(调用方需保证有空位)
// addr 已是块边界或落在块内返回 0; addr 落在空洞里返回 -1
static int split_block_at(struct free_table *ft, uintptr_t addr)
{
    int i;
    for (i = 0; i < ft->free; i++)
    {
        uintptr_t block_start = ft->table[i].start;
        uintptr_t block_end   = block_start + ft->table[i].size;
        if (addr == block_start)
        {
            return 0; // 已是边界
        }
        if (addr < block_start)
        {
            return -1; // 落在空洞里
        }
        if (addr < block_end)
        {
            // block_start < addr < block_end: 切分
            move_backward(ft, i + 1);
            ft->table[i + 1]       = ft->table[i];
            ft->table[i + 1].start = addr;
            ft->table[i + 1].size  = block_end - addr;
            ft->table[i].size      = addr - block_start;
            return 0;
        }
        if (addr == block_end)
        {
            return 0;
        }
    }
    return -1;
}

// 从表中摘除一个区间
// flags > 0 时要求所在块的 flags 一致; flags = 0 时不关心类型; flags < 0 返回 -EINVAL
// 失败时返回错误且不改动表; 缩容失败不算失败(保持原容量, 等下一次调用再试)
int ft_remove(struct free_table *ft, uintptr_t start, size_t size, int flags)
{
    if (flags < 0)
    {
        return -EINVAL;
    }
    if (size == 0 || start + size < start)
    {
        return -EINVAL;
    }
    int i = find_contain_block(ft, start, size);
    if (i < 0)
    {
        return -ENOENT;
    }
    if (flags != 0 && ft->table[i].flags != flags)
    {
        return -EINVAL;
    }

    uintptr_t end         = start + size;
    uintptr_t block_start = ft->table[i].start;
    uintptr_t block_end   = block_start + ft->table[i].size;

    if (start == block_start && end == block_end)
    {
        move_forward(ft, i + 1);
        // 摘除后左右两块可能变得相邻, 需要合并
        combine(ft, i);
        shrink(ft);
        return 0;
    }

    if (start == block_start)
    {
        ft->table[i].start = end;
        ft->table[i].size -= size;
        shrink(ft);
        return 0;
    }

    if (start > block_start && end < block_end)
    {
        if (ft->free >= ft->capacity)
        {
            if (grow(ft) < 0 || ft->free >= ft->capacity)
            {
                return -ENOMEM;
            }
        }
        ft->table[i].size = start - block_start;

        move_backward(ft, i + 1);
        ft->table[i + 1].start = end;
        ft->table[i + 1].size  = block_end - end;
        ft->table[i + 1].flags = ft->table[i].flags;
        return 0;
    }
    if (end == block_end)
    {
        ft->table[i].size -= size;
        shrink(ft);
        return 0;
    }
    return -EINVAL;
}

// flags > 0 时只从 flags 一致的块里分配; flags = 0 时不关心类型; flags < 0 返回 -EINVAL
intptr_t ft_allocate(struct free_table *ft, size_t size, int flags)
{
    if (flags < 0)
    {
        return -EINVAL;
    }
    intptr_t  ret = -ENOMEM;
    uintptr_t start;
    int       status = 0;
    int       i;
    for (i = 0; i < ft->free; i++)
    {
        if (ft->table[i].size < size)
        {
            continue;
        }
        if (flags != 0 && ft->table[i].flags != flags)
        {
            continue;
        }
        start  = ft->table[i].start;
        status = ft_remove(ft, start, size, flags);
        if (status == 0)
        {
            ret = (intptr_t)start;
            break;
        }
    }
    return ret;
}

// 地址是否在区间内, 且所在块的 flags 匹配
// flags > 0 时要求所在块的 flags 一致; flags = 0 时不关心类型; flags < 0 返回 0
int ft_find(struct free_table *ft, uintptr_t start, int flags)
{
    if (flags < 0)
    {
        return 0;
    }
    int i;
    for (i = 0; i < ft->free; i++)
    {
        uintptr_t block_start = ft->table[i].start;
        uintptr_t block_end   = block_start + ft->table[i].size;
        if (block_start > start || start >= block_end)
        {
            continue;
        }
        if (flags != 0 && ft->table[i].flags != flags)
        {
            continue;
        }
        return 1;
    }
    return 0;
}

// 查询 [start, start + size) 所在区间的 flags
// 成功返回 flags(>=0); 区间跨越了块的边界返回 -EINVAL; 找不到返回 -ENOENT
int ft_query_flags(struct free_table *ft, uintptr_t start, size_t size)
{
    if (size == 0 || start + size < start)
    {
        return -EINVAL;
    }
    int i;
    for (i = 0; i < ft->free; i++)
    {
        uintptr_t block_start = ft->table[i].start;
        uintptr_t block_end   = block_start + ft->table[i].size;
        if (block_start > start || start >= block_end)
        {
            continue;
        }
        if (start + size > block_end)
        {
            return -EINVAL; // 区间跨越了块的边界(进入另一种类型/空洞)
        }
        return ft->table[i].flags;
    }
    return -ENOENT;
}

// 修改 [start, start + size) 区间的 flags(就地切分)
// 区间必须已被表中区间覆盖(可跨多个相邻的不同类型区间): 不存在返回 -ENOENT
// 修改后相邻且 flags 相同的区间会被合并
// flags < 0 返回 -EINVAL
int ft_set_flags(struct free_table *ft, uintptr_t start, size_t size, int flags)
{
    if (flags < 0)
    {
        return -EINVAL;
    }
    if (size == 0 || start + size < start)
    {
        return -EINVAL;
    }
    // 区间必须被相邻的区间完整覆盖
    if (!is_range_covered(ft, start, size))
    {
        return -ENOENT;
    }

    // 预扩容: start/end 两处切分最多各需要一个空位
    while (ft->capacity - ft->free < 2)
    {
        if (grow(ft) < 0)
        {
            return -ENOMEM;
        }
    }

    uintptr_t end = start + size;

    // 让 start 与 end 成为块的边界(区间已校验存在, 不应失败)
    int split_status = split_block_at(ft, start);
    if (split_status < 0)
    {
        PANIC("ft_set_flags: Cannot split the block at the range start.");
    }
    split_status = split_block_at(ft, end);
    if (split_status < 0)
    {
        PANIC("ft_set_flags: Cannot split the block at the range end.");
    }

    // 把覆盖 [start, end) 的整块改为新 flags
    int i;
    for (i = 0; i < ft->free; i++)
    {
        uintptr_t block_start = ft->table[i].start;
        if (block_start < start || block_start >= end)
        {
            continue;
        }
        ft->table[i].flags = flags;
    }

    // 合并相邻的同 flags 区间
    for (i = 0; i < ft->free - 1;)
    {
        int before = ft->free;
        combine_with_next(ft, i);
        if (ft->free == before)
        {
            i++;
        }
    }
    return 0;
}

// 复制free_table,dst必须是空表
int copy_free_table(struct free_table *dst, struct free_table *src)
{
    if (dst->free != 0)
    {
        return -EINVAL;
    }
    uintptr_t block_start;
    size_t    block_size;
    int       block_flags;

    int i;
    for (i = 0; i < src->free; i++)
    {
        block_start = src->table[i].start;
        block_size  = src->table[i].size;
        block_flags = src->table[i].flags;
        int ret     = ft_add(dst, block_start, block_size, block_flags);
        if (ret < 0)
        {
            return ret;
        }
    }
    return 0;
}