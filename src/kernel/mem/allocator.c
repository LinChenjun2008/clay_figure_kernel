#include <kernel/global.h>
#include <mem/mem.h>
#include <lib/list.h>
#include <intr.h>
#include <std/string.h>
#include <device/spinlock.h>

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

PRIVATE mem_group_t mem_groups[NUMBER_OF_MEMORY_BLOCK_TYPES];

PUBLIC void mem_alloctor_init()
{
    size_t block_size = MIN_ALLOCATE_MEMORY_SIZE;
    int i;
    for (i = 0;i < NUMBER_OF_MEMORY_BLOCK_TYPES;i++)
    {
        mem_groups[i].block_size            = block_size;
        mem_groups[i].total_free            = 0;
        list_init(&mem_groups[i].free_block_list);
        init_spinlock(&mem_groups[i].lock);
        block_size *= 2;
    }
    return;
}

PRIVATE mem_block_t* cache2block(mem_cache_t *c,int idx)
{
    addr_t addr = (addr_t)c + sizeof(*c) + (PG_SIZE - sizeof(*c))
                    % c->group->block_size;
    return ((mem_block_t*)(addr + (idx * (c->group->block_size))));
}

PRIVATE mem_cache_t* block2cache(mem_block_t *b)
{
    return ((mem_cache_t*)((uintptr_t)b & 0xffffffffffe00000));
}

PUBLIC status_t pmalloc(size_t size,void *addr)
{
    if (addr == NULL)
    {
        return K_ERROR;
    }
    int i;
    mem_cache_t *c;
    mem_block_t *b;
    if (size > MAX_ALLOCATE_MEMORY_SIZE)
    {
        return K_ERROR;
    }
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
            pr_log("\3 %s:Out of Memory.\n",__func__);
            return K_ERROR;
        }
        c = KADDR_P2V(c);
        memset(c,0,PG_SIZE);
        c->group            = &mem_groups[i];
        c->number_of_blocks = (PG_SIZE - sizeof(*c)
                              - (PG_SIZE - sizeof(*c)) % c->group->block_size)
                              / c->group->block_size;
        c->cnt              = c->number_of_blocks;
        mem_groups[i].total_free += c->cnt;
        size_t block_index;
        for (block_index = 0;block_index < c->cnt;block_index++)
        {
            b = cache2block(c,block_index);
            list_append(&c->group->free_block_list,b);
        }
    }
    b = list_pop(&mem_groups[i].free_block_list);
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
        return;
    }
    mem_cache_t *c;
    mem_block_t *b;
    b = KADDR_P2V(addr);
    c = block2cache(b);
    mem_group_t *g = c->group;
    spinlock_lock(&g->lock);
    list_append(&g->free_block_list,b);
    c->cnt++;
    if (c->cnt == c->number_of_blocks && g->total_free >= c->number_of_blocks)
    {
        size_t idx;
        for (idx = 0;idx < c->number_of_blocks;idx++)
        {
            b = cache2block(c,idx);
            list_remove(b);
        }
        free_physical_page(KADDR_V2P(c),1);
    }
    spinlock_unlock(&g->lock);
    return;
}