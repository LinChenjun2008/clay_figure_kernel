// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/spinlock.h> // spinlock
#include <intr.h>            // intr functions
#include <lib/list.h>        // list functions
#include <mem/allocator.h>   // MIN,MAX allocate size
#include <mem/page.h>        // PHYS_TO_VIRT,VIRT_TO_PHYS
#include <std/string.h>      // memset

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

typedef struct mem_block_s
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

PRIVATE mem_group_t mem_groups[NUMBER_OF_MEMORY_BLOCK_TYPES];

PUBLIC void mem_allocator_init(void)
{
    size_t block_size = MIN_ALLOCATE_MEMORY_SIZE;
    int    i;
    for (i = 0; i < NUMBER_OF_MEMORY_BLOCK_TYPES; i++)
    {
        mem_groups[i].block_size = block_size;
        mem_groups[i].total_free = 0;
        list_init(&mem_groups[i].free_block_list);
        init_spinlock(&mem_groups[i].lock);
        block_size <<= 1;
    }
    return;
}

PRIVATE mem_block_t *cache2block(mem_cache_t *c, size_t idx)
{
    uintptr_t addr = (uintptr_t)c + c->group->block_size;
    return ((mem_block_t *)(addr + (idx * (c->group->block_size))));
}

PRIVATE mem_cache_t *block2cache(mem_block_t *b)
{
    return ((mem_cache_t *)((uintptr_t)b & ~((uintptr_t)PG_SIZE - 1)));
}

PRIVATE size_t block_index(mem_cache_t *c, mem_block_t *b)
{
    uintptr_t addr = (uintptr_t)b;
    addr -= (uintptr_t)c + c->group->block_size;
    size_t idx = addr / c->group->block_size;
    return idx;
}

PRIVATE mem_block_t *
kmalloc_find_block(list_t *list, size_t size, size_t alignment, size_t boundary)
{
    list_node_t *node = list->head.next;
    ASSERT(node->next->prev == node);
    if (alignment <= size && boundary == 0)
    {
        goto done;
    }
    for (; node != &list->tail; node = list_next(node))
    {
        mem_block_t *b   = CONTAINER_OF(mem_block_t, node, node);
        mem_cache_t *c   = block2cache(b);
        size_t       idx = block_index(c, b);
        // Check magic
        ASSERT(b->magic == idx + c->number_of_blocks);

        if (b->magic != idx + c->number_of_blocks)
        {
            PR_LOG(
                LOG_WARN, "Block magic error (memory may use after free).\n"
            );
        }

        ASSERT(node->next->prev == node);
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
        goto done;
    }
    return NULL;

done:
    list_remove(node);
    mem_block_t *b = CONTAINER_OF(mem_block_t, node, node);
    return b;
}

PUBLIC status_t
kmalloc(size_t size, size_t alignment, size_t boundary, void *addr)
{
    status_t status = K_SUCCESS;
    ASSERT(addr != NULL);
    if (alignment < size)
    {
        alignment = size;
    }
    int          i;
    mem_cache_t *c = NULL;
    mem_block_t *b = NULL;
    mem_group_t *g = NULL;

    if (size > MAX_ALLOCATE_MEMORY_SIZE)
    {
        return K_ERROR;
    }

    for (i = 0; i < NUMBER_OF_MEMORY_BLOCK_TYPES; i++)
    {
        if (size <= mem_groups[i].block_size)
        {
            g = &mem_groups[i];
            break;
        }
    }

    spinlock_lock(&g->lock);
    if (list_empty(&g->free_block_list))
    {
        uintptr_t cache_paddr;
        status = alloc_physical_page(1, &cache_paddr);
        if (ERROR(status))
        {
            goto done;
        }
        c = PHYS_TO_VIRT(cache_paddr);
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

    b = kmalloc_find_block(&g->free_block_list, size, alignment, boundary);
    if (b == NULL)
    {
        PR_LOG(LOG_WARN, "Can not find avilable memory block.\n");
        return K_NOT_FOUND;
    }
    memset(b, 0, g->block_size);
    c = block2cache(b);
    c->cnt--;
    c->group->total_free--;
    *(uintptr_t *)addr = (uintptr_t)b;
done:
    spinlock_unlock(&g->lock);
    return status;
}

PUBLIC void kfree(void *addr)
{
    if (addr == NULL)
    {
        PR_LOG(LOG_WARN, "free nullptr.\n");
        return;
    }
    mem_cache_t *c = NULL;
    mem_block_t *b = NULL;

    b = (mem_block_t *)addr;
    c = block2cache(b);

    b->magic = block_index(c, b) + c->number_of_blocks;

    mem_group_t *g = c->group;

    ASSERT(((uintptr_t)addr & (g->block_size - 1)) == 0);

    spinlock_lock(&g->lock);
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
        free_physical_page(VIRT_TO_PHYS(c), 1);
    }
    spinlock_unlock(&g->lock);
    return;
}