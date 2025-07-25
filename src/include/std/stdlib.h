#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <kernel/global.h>

#include <mem/allocator.h>
#include <mem/page.h>


static inline void *malloc(size_t size)
{
    void    *addr;
    status_t ret;
    ret = kmalloc(size, 0, 0, &addr);
    if (ret != K_SUCCESS)
    {
        return NULL;
    }
    return addr;
}

#define free(addr) kfree(addr)

#endif