/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <lib/fifo.h>

PUBLIC void init_fifo(fifo_t *fifo,void *buf,int type,int size)
{
    fifo->type = type;
    switch(type)
    {
        case 8:
            fifo->buf8 = buf;
            break;
        case 16:
            fifo->buf16 = buf;
            break;
        case 32:
            fifo->buf32 = buf;
            break;
        case 64:
            fifo->buf64 = buf;
            break;
    }
    fifo->size = size;
    fifo->free = size;
    fifo->nr = 0;
    fifo->nw = 0;
}


PUBLIC status_t fifo_put(fifo_t *fifo,void* data)
{
    if (data == NULL)
    {
        return K_INVAILD_ADDR;
    }

    if (fifo->free == 0) /* 没有空余 */
    {
        return K_OUT_OF_RESOURCE;
    }
    fifo->free--;
    switch(fifo->type)
    {
        case 8:
            fifo->buf8[fifo->nw] = *((uint8_t*)data);
            break;
        case 16:
            fifo->buf16[fifo->nw] = *((uint16_t*)data);
            break;
        case 32:
            fifo->buf32[fifo->nw] = *((uint32_t*)data);
            break;
        case 64:
            fifo->buf64[fifo->nw] = *((uint64_t*)data);
            break;
    }
    fifo->nw = (fifo->nw + 1) % fifo->size;
    return K_SUCCESS;
}

PUBLIC status_t fifo_get(fifo_t *fifo,void* data)
{
    if (data == NULL)
    {
        return K_INVAILD_ADDR;
    }
    if (fifo->free == fifo->size)
    {
        return K_OUT_OF_RESOURCE;
    }
    fifo->free++;
    switch(fifo->type)
    {
        case 8:
            *((uint8_t*)data) = fifo->buf8[fifo->nr];
            break;
        case 16:
            *((uint16_t*)data) = fifo->buf16[fifo->nr];
            break;
        case 32:
            *((uint32_t*)data) = fifo->buf32[fifo->nr];
            break;
        case 64:
            *((uint64_t*)data) = fifo->buf64[fifo->nr];
            break;
    }
    fifo->nr = (fifo->nr + 1) % fifo->size;
    return K_SUCCESS;
}


PUBLIC bool fifo_empty(fifo_t *fifo)
{
    return (fifo->free == fifo->size);
}

PUBLIC bool fifo_fill(fifo_t *fifo)
{
    return (fifo->free == 0);
}