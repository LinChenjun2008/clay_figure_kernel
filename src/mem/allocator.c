// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/mem/page.h>
#include <asm/sync/spinlock.h>

#include <lib/list.h>
#include <mem/allocator.h>
#include <print.h>
#include <std/string.h>

typedef struct
{
    size_t     block_size;
    uint32_t   total_free;
    list_t     free_block_list;
    spinlock_t lock;
} mem_group_t;

typedef struct
{
    mem_group_t *group;
    uint64_t     number_of_blocks;
    size_t       cnt;
} mem_cache_t;

typedef struct
{
    uint64_t    magic; // magic = block_index + cache.number_of_blocks
    list_node_t node;
} mem_block_t;

STATIC_ASSERT(sizeof(mem_cache_t) <= MIN_ALLOCATE_MEMORY_SIZE, "");
STATIC_ASSERT(sizeof(mem_block_t) <= MIN_ALLOCATE_MEMORY_SIZE, "");

STATIC_ASSERT(
    MIN_ALLOCATE_MEMORY_SIZE << (NUMBER_OF_MEMORY_BLOCK_TYPES - 1) ==
        MAX_ALLOCATE_MEMORY_SIZE,
    ""
);
STATIC_ASSERT(MAX_ALLOCATE_MEMORY_SIZE < PG_SIZE, "");

static mem_group_t mem_groups[NUMBER_OF_MEMORY_BLOCK_TYPES];

void mem_allocator_init(void)
{
    size_t block_size = MIN_ALLOCATE_MEMORY_SIZE;
    int    i;
    for (i = 0; i < NUMBER_OF_MEMORY_BLOCK_TYPES; i++)
    {
        mem_groups[i].block_size = block_size;
        mem_groups[i].total_free = 0;
        init_list(&mem_groups[i].free_block_list);
        init_spinlock(&mem_groups[i].lock);
        block_size <<= 1;
    }
    return;
}


static mem_block_t *cache2block(mem_cache_t *c, size_t idx)
{
    uintptr_t addr = (uintptr_t)c + c->group->block_size;
    return ((mem_block_t *)(addr + (idx * (c->group->block_size))));
}

static mem_cache_t *block2cache(mem_block_t *b)
{
    return ((mem_cache_t *)((uintptr_t)b & ~((uintptr_t)PG_SIZE - 1)));
}

static size_t block_index(mem_cache_t *c, mem_block_t *b)
{
    uintptr_t addr = (uintptr_t)b;
    addr -= (uintptr_t)c + c->group->block_size;
    size_t idx = addr / c->group->block_size;
    return idx;
}

static mem_block_t *
find_block(mem_group_t *g, size_t alignment, size_t boundary)
{
    list_t *list = &g->free_block_list;
    size_t  size = g->block_size;

    list_node_t *node = list_next(list_head(list));
    ASSERT(list_prev(list_next(node)) == node);

    mem_block_t *b   = NULL;
    mem_cache_t *c   = NULL;
    size_t       idx = 0;

    if (alignment <= size && boundary == 0)
    {
        b = CONTAINER_OF(mem_block_t, node, node);
        list_remove(node);
        return b;
    }
    for (; node != list_tail(list); node = list_next(node))
    {
        b   = CONTAINER_OF(mem_block_t, node, node);
        c   = block2cache(b);
        idx = block_index(c, b);
        // Check magic
        ASSERT(b->magic == idx + c->number_of_blocks);

        if (b->magic != idx + c->number_of_blocks)
        {
            printk(MSG_WARN "kmalloc: block magic error.\n");
        }

        ASSERT(list_prev(list_next(node)) == node);
        uintptr_t addr = (uintptr_t)b;
        if ((addr & (alignment - 1)) != 0)
        {
            continue;
        }
        if (boundary != 0)
        {
            if (addr / boundary != (addr + size - 1) / boundary)
            {
                continue;
            }
        }

        list_remove(node);
        return b;
    }
    return NULL;
}

static mem_group_t *size_to_group(size_t size)
{
    int i;
    for (i = 0; i < NUMBER_OF_MEMORY_BLOCK_TYPES; i++)
    {
        if (size <= mem_groups[i].block_size)
        {
            return &mem_groups[i];
        }
    }
    return NULL;
}

static int
kmalloc_lock(mem_group_t *g, size_t alignment, size_t boundary, void **addr)
{
    int ret = 0;

    mem_cache_t *c = NULL;
    mem_block_t *b = NULL;

    if (list_empty(&g->free_block_list))
    {
        ret = allocate_pages(1, (void **)&c);

        if (ret < 0)
        {
            printk(MSG_ERR "kmalloc: failed to allocate page.\n");
            return 0;
        }
        memset(c, 0, PG_SIZE);

        c->group            = g;
        c->number_of_blocks = PG_SIZE / c->group->block_size - 1;
        c->cnt              = c->number_of_blocks;
        c->group->total_free += c->cnt;
        size_t block_index;
        for (block_index = 0; block_index < c->cnt; block_index++)
        {
            b = cache2block(c, block_index);
            list_append(&c->group->free_block_list, &b->node);
            // Set Magic
            b->magic = block_index + c->number_of_blocks;
        }
    }

    b = find_block(g, alignment, boundary);
    if (b == NULL)
    {
        printk(MSG_WARN "kmalloc: can not find avilable memory block.\n");
        return -1;
    }
    memset(b, 0, g->block_size);
    c = block2cache(b);
    c->cnt--;
    c->group->total_free--;
    *addr = (void *)b;
    return ret;
}

int kmalloc(size_t size, size_t alignment, size_t boundary, void **addr)
{
    int ret = 0;
    if (size < MAX_ALLOCATE_MEMORY_SIZE && alignment > MAX_ALLOCATE_MEMORY_SIZE)
    {
        return -1;
    }
    if ((alignment & (alignment - 1)) != 0)
    {
        return -2;
    }

    mem_cache_t *c = NULL;
    mem_group_t *g = NULL;

    // 超过最大分配内存大小，按页为单位分配
    if (size > MAX_ALLOCATE_MEMORY_SIZE)
    {
        size_t pages = DIV_ROUND_UP(sizeof(*c) + alignment + size, PG_SIZE);

        ret = allocate_pages(pages, (void **)&c);
        if (ret < 0)
        {
            return ret;
        }
        c->group            = NULL;
        c->cnt              = pages;
        c->number_of_blocks = 0;

        uintptr_t raw_ptr = (uintptr_t)c + sizeof(*c);
        if (alignment != 0)
        {
            raw_ptr = (raw_ptr + alignment - 1) & ~(alignment - 1);
        }
        *addr = (void *)raw_ptr;
        return ret;
    }
    g = size_to_group(size);
    ASSERT(g != NULL);

    spin_lock(&g->lock);
    ret = kmalloc_lock(g, alignment, boundary, addr);
    spin_unlock(&g->lock);

    return ret;
}

void kfree(void **addr)
{
    if (addr == NULL)
    {
        printk(MSG_ERR "kfree: bad pargma.\n");
        return;
    }
    if (*addr == NULL)
    {
        printk(MSG_WARN "kfree: free nullptr.\n");
        return;
    }
    mem_cache_t *c = NULL;
    mem_block_t *b = NULL;
    mem_group_t *g = NULL;

    b = (mem_block_t *)*addr;
    c = block2cache(b);
    g = c->group;

    // 先处理大块内存
    if (c->group == NULL)
    {
        free_pages((void **)&c, c->cnt);
        if (c == NULL)
        {
            *addr = NULL;
        }
        return;
    }
    // 小块内存
    if (b->magic == block_index(c, b) + c->number_of_blocks)
    {
        printk(MSG_WARN "kfree: double free: %p.\n", b);
        return;
    }
    b->magic = block_index(c, b) + c->number_of_blocks;

    ASSERT(((uintptr_t)*addr & (g->block_size - 1)) == 0);

    spin_lock(&g->lock);
    list_append(&g->free_block_list, &b->node);
    g->total_free++;
    c->cnt++;
    if (c->cnt == c->number_of_blocks)
    {
        size_t idx;
        for (idx = 0; idx < c->number_of_blocks; idx++)
        {
            b = cache2block(c, idx);
            // Check magic
            ASSERT(b->magic == idx + c->number_of_blocks);

            list_remove(&b->node);
        }
        g->total_free -= c->number_of_blocks;
        free_pages((void **)&c, 1);
    }
    spin_unlock(&g->lock);
    *addr = NULL;
    return;
}