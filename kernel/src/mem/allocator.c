// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <mem.h>
#include <mem/struct.h>
#include <print.h>
#include <std/string.h>

static struct mem_group mem_groups[MAX_BLOCK_TYPES];

void mem_allocator_init(void)
{
    size_t block_size = MIN_BLOCK_SIZE;

    int i;
    for (i = 0; i < MAX_BLOCK_TYPES; i++)
    {
        mem_groups[i].block_size = block_size;
        mem_groups[i].total_free = 0;
        init_list(&mem_groups[i].free_block_list);
        init_spinlock(&mem_groups[i].lock);
        block_size *= 2;
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
    for (i = 0; i < MAX_BLOCK_TYPES; i++)
    {
        if (size <= mem_groups[i].block_size)
        {
            return &mem_groups[i];
        }
    }
    return NULL;
}

static void *
kmalloc_find(struct mem_group *g, size_t alignment, size_t boundary)
{
    struct mem_cache *c = NULL;
    struct mem_block *b = NULL;
    if (list_empty(&g->free_block_list))
    {
        c = allocate_pages(1);
        if (c == NULL)
        {
            printk(MSG_ERR "kmalloc: allocate page failed.\n");
            return NULL;
        }
        c->group            = g;
        c->number_of_blocks = PG_SIZE / c->group->block_size - 1;
        c->count            = c->number_of_blocks;
        c->group->total_free += c->count;
        size_t block_index;
        for (block_index = 0; block_index < c->count; block_index++)
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
        return NULL;
    }
    c = block2cache(b);
    c->count--;
    c->group->total_free--;
    return b;
}

void *kmalloc(size_t size, size_t alignment, size_t boundary)
{
    if (size < MAX_BLOCK_SIZE && alignment > MAX_BLOCK_SIZE)
    {
        return NULL;
    }
    if ((alignment & (alignment - 1)) != 0)
    {
        return NULL;
    }
    struct mem_cache *c = NULL;
    struct mem_group *g = NULL;
    if (size > MAX_BLOCK_SIZE)
    {
        size_t pages = DIV_ROUND_UP(sizeof(*c) + alignment + size, PG_SIZE);

        c = allocate_pages(pages);
        if (c == NULL)
        {
            return NULL;
        }
        c->group            = NULL;
        c->count            = pages;
        c->number_of_blocks = 0;
        uintptr_t ptr       = (uintptr_t)c + sizeof(*c);
        if (alignment != 0)
        {
            ptr = (ptr + alignment - 1) & ~(alignment - 1);
        }
        return (void *)ptr;
    }
    g = size_to_group(size);
    ASSERT(g != NULL);

    void *ret = NULL;
    spin_lock(&g->lock);
    ret = kmalloc_find(g, alignment, boundary);
    spin_unlock(&g->lock);

    return ret;
}

void kfree(void *addr)
{
    if (addr == NULL)
    {
        printk(MSG_ERR "kfree: free nullptr.\n");
        return;
    }
    struct mem_cache *c = NULL;
    struct mem_block *b = NULL;
    struct mem_group *g = NULL;

    b = (struct mem_block *)addr;
    c = block2cache(b);
    g = c->group;

    if (c->group == NULL)
    {
        free_pages(c, c->count);
        if (c == NULL)
        {
            addr = NULL;
        }
        return;
    }
    if (b->magic == block_index(c, b) + c->number_of_blocks)
    {
        printk(MSG_WARN "kfree: double free: %p.\n", b);
        return;
    }
    b->magic = block_index(c, b) + c->number_of_blocks;

    ASSERT(((uintptr_t)addr & (g->block_size - 1)) == 0);

    spin_lock(&g->lock);
    list_append(&g->free_block_list, &b->node);
    g->total_free++;
    c->count++;
    if (c->count == c->number_of_blocks)
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
        free_pages(c, 1);
    }
    spin_unlock(&g->lock);
    return;
}