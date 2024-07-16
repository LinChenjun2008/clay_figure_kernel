#include <kernel/global.h>
#include <intr.h>
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


PUBLIC int fifo_put(fifo_t *fifo,void* data)
{
    intr_status_t intr_status = intr_disable();

    if (fifo->free == 0) /* 没有空余 */
    {
        return K_ERROR;
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
    intr_set_status(intr_status);
    return K_SUCCESS;
}

PUBLIC int fifo_get(fifo_t *fifo,void* data)
{
    intr_status_t intr_status = intr_disable();
    if (fifo->free == fifo->size) /* 缓冲区是空的 */
    {
        return K_ERROR;
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
    intr_set_status(intr_status);
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