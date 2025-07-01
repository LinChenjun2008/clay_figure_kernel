/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#include <kernel/global.h>

#include <log.h>

#include <device/spinlock.h> // spinlock
#include <intr.h>            // intr functions
#include <lib/list.h>        // list functions
#include <mem/allocator.h>   // MIN,MAX allocate size
#include <mem/page.h>        //  KADDR_P2V,KADDR_V2P
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

STATIC_ASSERT(MAX_ALLOCATE_MEMORY_SIZE < PG_SIZE, "");

PRIVATE mem_group_t mem_groups[NUMBER_OF_MEMORY_BLOCK_TYPES];

PUBLIC void mem_allocator_init(void)
{
    ASSERT(MIN_ALLOCATE_MEMORY_SIZE >= sizeof(mem_block_t));
    size_t block_size = MIN_ALLOCATE_MEMORY_SIZE;
    int    i;
    for (i = 0; i < NUMBER_OF_MEMORY_BLOCK_TYPES; i++)
    {
        ASSERT(!(block_size & (block_size - 1)));
        mem_groups[i].block_size = block_size;
        mem_groups[i].total_free = 0;
        list_init(&mem_groups[i].free_block_list);
        init_spinlock(&mem_groups[i].lock);
        block_size *= 2;
    }
    ASSERT(block_size == MAX_ALLOCATE_MEMORY_SIZE);
    return;
}

PRIVATE mem_block_t *cache2block(mem_cache_t *c, size_t idx)
{
    addr_t addr = (addr_t)c + c->group->block_size;
    return ((mem_block_t *)(addr + (idx * (c->group->block_size))));
}

PRIVATE mem_cache_t *block2cache(mem_block_t *b)
{
    return ((mem_cache_t *)((uintptr_t)b & ~((uintptr_t)PG_SIZE - 1)));
}

PRIVATE size_t block_index(mem_cache_t *c, mem_block_t *b)
{
    addr_t addr = (addr_t)b;
    addr -= (addr_t)c + c->group->block_size;
    size_t idx = addr / c->group->block_size;
    return idx;
}

PRIVATE mem_block_t *
pmalloc_find_block(list_t *list, size_t size, size_t alignment, size_t boundary)
{
    list_node_t *res = list->head.next;
    ASSERT(res->next->prev == res);
    if (alignment <= size && boundary == 0)
    {
        goto done;
    }
    for (; res != &list->tail; res = list_next(res))
    {
        mem_block_t *b = CONTAINER_OF(mem_block_t, node, res);
        ASSERT(res->next->prev == res);
        addr_t addr = (addr_t)b;
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
    list_remove(res);
    mem_block_t *b = CONTAINER_OF(mem_block_t, node, res);
    return b;
}

PUBLIC status_t
pmalloc(size_t size, size_t alignment, size_t boundary, void *addr)
{
    status_t status = K_SUCCESS;
    ASSERT(addr != NULL);
    if (alignment < size)
    {
        alignment = size;
    }
    int          i;
    mem_cache_t *c;
    mem_block_t *b;
    ASSERT(size <= MAX_ALLOCATE_MEMORY_SIZE);

    if (size > MAX_ALLOCATE_MEMORY_SIZE)
    {
        return K_ERROR;
    }

    for (i = 0; i < NUMBER_OF_MEMORY_BLOCK_TYPES; i++)
    {
        if (size <= mem_groups[i].block_size)
        {
            break;
        }
    }

    spinlock_lock(&mem_groups[i].lock);
    if (list_empty(&mem_groups[i].free_block_list))
    {
        phy_addr_t cache_paddr;
        status = alloc_physical_page(1, &cache_paddr);
        if (ERROR(status))
        {
            ASSERT(!ERROR(status));
            goto done;
        }
        c = KADDR_P2V(cache_paddr);
        memset(c, 0, PG_SIZE);
        c->group            = &mem_groups[i];
        c->number_of_blocks = PG_SIZE / c->group->block_size - 1;
        c->cnt              = c->number_of_blocks;
        mem_groups[i].total_free += c->cnt;
        size_t block_index;
        for (block_index = 0; block_index < c->cnt; block_index++)
        {
            b = cache2block(c, block_index);
            list_append(&c->group->free_block_list, &b->node);
            // Set Magic
            b->magic = block_index + c->number_of_blocks;
            ASSERT(b->node.next != NULL && NULL != b->node.prev);
        }
    }

    b = pmalloc_find_block(
        &mem_groups[i].free_block_list, size, alignment, boundary
    );
    if (b == NULL)
    {
        PR_LOG(LOG_WARN, "Can not find avilable memory block.\n");
        return K_NOT_FOUND;
    }
    memset(b, 0, mem_groups[i].block_size);
    c = block2cache(b);
    c->cnt--;
    mem_groups[i].total_free--;
    *(phy_addr_t *)addr = (phy_addr_t)KADDR_V2P(b);
done:
    spinlock_unlock(&mem_groups[i].lock);
    return status;
}

PUBLIC void pfree(void *addr)
{
    if (addr == NULL)
    {
        PR_LOG(LOG_WARN, "free nullptr.\n");
        return;
    }
    mem_cache_t *c;
    mem_block_t *b;
    b = KADDR_P2V(addr);
    ASSERT(b != NULL);
    c = block2cache(b);

    // Set magic so that we can detect memory leak.
    b->magic       = block_index(c, b) + c->number_of_blocks;
    mem_group_t *g = c->group;
    ASSERT(((addr_t)addr & (g->block_size - 1)) == 0);
    spinlock_lock(&g->lock);
    list_append(&g->free_block_list, &b->node);
    ASSERT(b->node.next != NULL && NULL != b->node.prev);
    g->total_free++;
    c->cnt++;
    if (c->cnt == c->number_of_blocks)
    {
        size_t idx;
        for (idx = 0; idx < c->number_of_blocks; idx++)
        {
            b = cache2block(c, idx);
            ASSERT(b != NULL);
            ASSERT(b->node.next != NULL && NULL != b->node.prev);
            // Check magic
            ASSERT(b->magic == idx + c->number_of_blocks);

            if (b->magic != idx + c->number_of_blocks)
            {
                PR_LOG(
                    LOG_WARN, "Block magic error. Memory leaks may occur.\n"
                );
            }
            list_remove(&b->node);
        }
        g->total_free -= c->number_of_blocks;
        free_physical_page(KADDR_V2P(c), 1);
    }
    spinlock_unlock(&g->lock);
    return;
}