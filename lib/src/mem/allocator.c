// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <asm/page.h> // PG_SIZE

#include <user/lib.h>
#include <user/list.h>

#define MIN_BLOCK_SIZE  64   //   64 Byte
#define MAX_BLOCK_SIZE  2048 //    2 KiB
#define MAX_BLOCK_TYPES 6

#define OFFSET_OF(CONTAINER_TYPE, MEMBER_NAME) \
    ((uintptr_t)&((CONTAINER_TYPE *)0)->MEMBER_NAME)

#define CONTAINER_OF(CONTAINER_TYPE, MEMBER_NAME, MEMBER_PTR) \
    ((CONTAINER_TYPE *)((uintptr_t)(MEMBER_PTR) -             \
                        OFFSET_OF(CONTAINER_TYPE, MEMBER_NAME)))

struct mem_group
{
    size_t      block_size;
    size_t      total_free;
    struct list free_block_list;
};

struct mem_cache
{
    struct mem_group *group;            // NULL: 这是按页分配的大块内存
    size_t            number_of_blocks; // 大块内存时为 0
    size_t            count;            // 大块内存时为页数
};

struct mem_block
{
    uint64_t         magic;
    struct list_node node;
};

static struct mem_group groups[MAX_BLOCK_TYPES];

void lib_allocator_init(void)
{
    size_t block_size = MIN_BLOCK_SIZE;
    int    i;
    for (i = 0; i < MAX_BLOCK_TYPES; i++)
    {
        groups[i].block_size = block_size;
        groups[i].total_free = 0;
        lib_init_list(&groups[i].free_block_list);
        block_size *= 2;
    }
    return;
}

static struct mem_block *cache2block(struct mem_cache *c, size_t idx)
{
    uintptr_t addr = (uintptr_t)c + c->group->block_size;
    return ((struct mem_block *)(addr + idx * c->group->block_size));
}

static struct mem_cache *block2cache(struct mem_block *b)
{
    return ((struct mem_cache *)((uintptr_t)b & ~((uintptr_t)PG_SIZE - 1)));
}

static size_t block_index(struct mem_cache *c, struct mem_block *b)
{
    uintptr_t addr = (uintptr_t)b - ((uintptr_t)c + c->group->block_size);
    return addr / c->group->block_size;
}

static struct mem_group *size_to_group(size_t size)
{
    int i;
    for (i = 0; i < MAX_BLOCK_TYPES; i++)
    {
        if (size <= groups[i].block_size)
        {
            return &groups[i];
        }
    }
    return NULL;
}

static struct mem_cache *allocate_cache(struct mem_group *g)
{
    struct mem_cache *c = (struct mem_cache *)allocate_pages(NULL, 1);
    if (c == NULL)
    {
        return NULL;
    }
    c->group            = g;
    c->number_of_blocks = PG_SIZE / g->block_size - 1;
    c->count            = c->number_of_blocks;
    g->total_free += c->count;

    size_t idx;
    for (idx = 0; idx < c->number_of_blocks; idx++)
    {
        struct mem_block *b = cache2block(c, idx);
        lib_list_push(&g->free_block_list, &b->node);
        b->magic = idx + c->number_of_blocks;
    }
    return c;
}

static struct mem_block *find_block(struct mem_group *g)
{
    struct list_node *node = lib_list_pop(&g->free_block_list);
    if (node == NULL)
    {
        return NULL;
    }
    return CONTAINER_OF(struct mem_block, node, node);
}

static void *small_allocate(struct mem_group *g)
{
    struct mem_block *b = find_block(g);
    if (b == NULL)
    {
        if (allocate_cache(g) == NULL)
        {
            return NULL;
        }
        b = find_block(g);
        if (b == NULL)
        {
            return NULL;
        }
    }
    struct mem_cache *c = block2cache(b);
    c->count--;
    g->total_free--;
    return ((void *)b);
}

static void *large_allocate(size_t size)
{
    size_t pages = (MIN_BLOCK_SIZE + size + PG_SIZE - 1) / PG_SIZE;

    struct mem_cache *c = (struct mem_cache *)allocate_pages(NULL, pages);
    if (c == NULL)
    {
        return NULL;
    }
    c->group            = NULL;
    c->number_of_blocks = 0;
    c->count            = pages;

    return ((void *)((uintptr_t)c + MIN_BLOCK_SIZE));
}

void *malloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    if (size > MAX_BLOCK_SIZE)
    {
        return large_allocate(size);
    }
    struct mem_group *g = size_to_group(size);
    if (g == NULL)
    {
        return NULL;
    }
    return small_allocate(g);
}

void free(void *addr)
{
    if (addr == NULL)
    {
        return;
    }

    struct mem_block *b = (struct mem_block *)addr;
    struct mem_cache *c = block2cache(b);

    if (c->group == NULL)
    {
        free_pages(c, c->count);
        return;
    }
    struct mem_group *g = c->group;

    if (b->magic == block_index(c, b) + c->number_of_blocks)
    {
        return;
    }
    b->magic = block_index(c, b) + c->number_of_blocks;

    lib_list_push(&g->free_block_list, &b->node);
    g->total_free++;
    c->count++;

    if (c->count == c->number_of_blocks)
    {
        size_t idx;
        for (idx = 0; idx < c->number_of_blocks; idx++)
        {
            struct mem_block *block = cache2block(c, idx);
            lib_list_remove(&block->node);
        }
        g->total_free -= c->number_of_blocks;
        free_pages(c, 1);
    }
    return;
}