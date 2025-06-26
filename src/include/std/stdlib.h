#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <kernel/global.h>

#include <mem/mem.h>

static inline void *malloc(size_t size)
{
    void    *addr;
    status_t ret;
    ret = pmalloc(size, 0, 0, &addr);
    if (ret != K_SUCCESS)
    {
        return NULL;
    }
    return KADDR_P2V(addr);
}

#define free(addr) pfree(KADDR_V2P(addr))

#endif