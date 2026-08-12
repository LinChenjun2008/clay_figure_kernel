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

struct mem_group
{
    size_t          block_size;
    uint32_t        total_free;
    struct list     free_block_list;
    struct spinlock lock;
};

struct mem_cache
{
    struct mem_group *group;
    uint64_t          number_of_blocks;
    size_t            cnt;
};

struct mem_block
{
    uint64_t         magic; // magic = block_index + cache.number_of_blocks
    struct list_node node;
};

STATIC_ASSERT(sizeof(struct mem_cache) <= MIN_ALLOCATE_MEMORY_SIZE, "");
STATIC_ASSERT(sizeof(struct mem_block) <= MIN_ALLOCATE_MEMORY_SIZE, "");

STATIC_ASSERT(
    MIN_ALLOCATE_MEMORY_SIZE << (NUMBER_OF_MEMORY_BLOCK_TYPES - 1) ==
        MAX_ALLOCATE_MEMORY_SIZE,
    ""
);
STATIC_ASSERT(MAX_ALLOCATE_MEMORY_SIZE < PG_SIZE, "");

static struct mem_group mem_groups[NUMBER_OF_MEMORY_BLOCK_TYPES];

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


static struct mem_block *cache2block(struct mem_cache *c, size_t idx)
{
    uintptr_t addr = (uintptr_t)c + c->group->block_size;
    return ((struct mem_block *)(addr + (idx * (c->group->block_size))));
}

static struct mem_cache *block2cache(struct mem_block *b)
{
    return ((struct mem_cache *)((uintptr_t)b & ~((uintptr_t)PG_SIZE - 1)));
}

static size_t block_index(struct mem_cache *c, struct mem_block *b)
{
    uintptr_t addr = (uintptr_t)b;
    addr -= (uintptr_t)c + c->group->block_size;
    size_t idx = addr / c->group->block_size;
    return idx;
}

static struct mem_block *
find_block(struct mem_group *g, size_t alignment, size_t boundary)
{
    struct list *list = &g->free_block_list;
    size_t       size = g->block_size;

    struct list_node *node = list_next(list_head(list));
    ASSERT(list_prev(list_next(node)) == node);

    struct mem_block *b   = NULL;
    struct mem_cache *c   = NULL;
    size_t            idx = 0;

    if (alignment <= size && boundary == 0)
    {
        b = CONTAINER_OF(struct mem_block, node, node);
        list_remove(node);
        return b;
    }
    for (; node != list_tail(list); node = list_next(node))
    {
        b   = CONTAINER_OF(struct mem_block, node, node);
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

static struct mem_group *size_to_group(size_t size)
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

static int kmalloc_lock(
    struct mem_group *g,
    size_t            alignment,
    size_t            boundary,
    void            **addr
)
{
    int ret = 0;

    struct mem_cache *c = NULL;
    struct mem_block *b = NULL;

    if (list_empty(&g->free_block_list))
    {
        ret = allocate_pages(1, (void **)&c);

        if (ret < 0)
        {
            printk(MSG_ERR "kmalloc: failed to allocate page.\n");
            return ret;
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
        printk(MSG_WARN "kmalloc: can not find available memory block.\n");
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

    struct mem_cache *c = NULL;
    struct mem_group *g = NULL;

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
    memset(*addr, 0, size);
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
    struct mem_cache *c = NULL;
    struct mem_block *b = NULL;
    struct mem_group *g = NULL;

    b = (struct mem_block *)*addr;
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