/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <mem/mem.h>         // memory functions
#include <lib/list.h>        // list functions
#include <intr.h>            // intr functions
#include <std/string.h>      // memset
#include <device/spinlock.h> // spinlock

#include <log.h>

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

typedef list_node_t mem_block_t;

STATIC_ASSERT(sizeof (mem_cache_t) <= MIN_ALLOCATE_MEMORY_SIZE,"");
STATIC_ASSERT(sizeof (mem_block_t) <= MIN_ALLOCATE_MEMORY_SIZE,"");

PRIVATE mem_group_t mem_groups[NUMBER_OF_MEMORY_BLOCK_TYPES];

PUBLIC void mem_allocator_init(void)
{
    ASSERT(MIN_ALLOCATE_MEMORY_SIZE >= sizeof(mem_block_t));
    size_t block_size = MIN_ALLOCATE_MEMORY_SIZE;
    int i;
    for (i = 0;i < NUMBER_OF_MEMORY_BLOCK_TYPES;i++)
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

PRIVATE mem_block_t* cache2block(mem_cache_t *c,size_t idx)
{
    addr_t addr = (addr_t)c + c->group->block_size;
    return ((mem_block_t*)(addr + (idx * (c->group->block_size))));
}

PRIVATE mem_cache_t* block2cache(mem_block_t *b)
{
    return ((mem_cache_t*)((uintptr_t)b & 0xffffffffffe00000));
}

PRIVATE mem_block_t *pmalloc_find_block(list_t *list,
                                        size_t size,
                                        size_t alignment,
                                        size_t boundary)
{
    list_node_t *res = list->head.next;
    if (alignment <= size && boundary == 0)
    {
        list_remove(res);
        return res;
    }
    for (;res != &list->tail;res = list_next(res))
    {
        addr_t addr = (addr_t)res;
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
        list_remove(res);
        return res;
    }
    return NULL;
}

PUBLIC status_t pmalloc(size_t size,size_t alignment,size_t boundary,void *addr)
{
    ASSERT(addr != NULL);
    if (alignment < size)
    {
        alignment = size;
    }
    int i;
    mem_cache_t *c;
    mem_block_t *b;
    ASSERT(size <= MAX_ALLOCATE_MEMORY_SIZE);

    for (i = 0;i < NUMBER_OF_MEMORY_BLOCK_TYPES;i++)
    {
        if (size <= mem_groups[i].block_size)
        {
            break;
        }
    }

    spinlock_lock(&mem_groups[i].lock);
    if (list_empty(&mem_groups[i].free_block_list))
    {
        status_t status = alloc_physical_page(1,&c);
        if (ERROR(status))
        {
            spinlock_unlock(&mem_groups[i].lock);
            ASSERT(!ERROR(status));
            return K_NOMEM;
        }
        c = KADDR_P2V(c);
        memset(c,0,PG_SIZE);
        c->group            = &mem_groups[i];
        c->number_of_blocks = PG_SIZE / c->group->block_size - 1;
        c->cnt              = c->number_of_blocks;
        mem_groups[i].total_free += c->cnt;
        size_t block_index;
        for (block_index = 0;block_index < c->cnt;block_index++)
        {
            b = cache2block(c,block_index);
            list_append(&c->group->free_block_list,b);
            ASSERT(b->next != NULL && NULL != b->prev);
        }
    }

    b = pmalloc_find_block(&mem_groups[i].free_block_list,
                           size,
                           alignment,
                           boundary);
    if (b == NULL)
    {
        return K_NOT_FOUND;
    }
    memset(b,0,mem_groups[i].block_size);
    c = block2cache(b);
    c->cnt--;
    mem_groups[i].total_free--;
    spinlock_unlock(&mem_groups[i].lock);
    *(phy_addr_t*)addr = (phy_addr_t)KADDR_V2P(b);
    return K_SUCCESS;
}

PUBLIC void pfree(void *addr)
{
    if (addr == NULL)
    {
        pr_log("\3 %s: free nullptr.\n",__func__);
        return;
    }
    mem_cache_t *c;
    mem_block_t *b;
    b = KADDR_P2V(addr);
    ASSERT(b != NULL);
    c = block2cache(b);
    mem_group_t *g = c->group;
    ASSERT(((addr_t)addr & (g->block_size - 1)) == 0);
    spinlock_lock(&g->lock);
    list_append(&g->free_block_list,b);
    ASSERT(b->next != NULL && NULL != b->prev);

    c->cnt++;
    if (c->cnt == c->number_of_blocks)
    {
        size_t idx;
        for (idx = 0;idx < c->number_of_blocks;idx++)
        {
            b = cache2block(c,idx);
            ASSERT(b != NULL);
            ASSERT(b->next != NULL && NULL != b->prev);
            list_remove(b);
        }
        g->total_free -= c->number_of_blocks;
        free_physical_page(KADDR_V2P(c),1);
    }
    spinlock_unlock(&g->lock);
    return;
}