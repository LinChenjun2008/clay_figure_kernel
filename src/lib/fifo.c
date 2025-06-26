/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#include <kernel/global.h>

#include <device/spinlock.h>
#include <lib/fifo.h>
#include <std/string.h>

PUBLIC void init_fifo(fifo_t *fifo, void *data, size_t item_size, int size)
{
    fifo->data       = data;
    fifo->item_size  = item_size;
    fifo->size       = size;
    fifo->free       = size;
    fifo->next_read  = 0;
    fifo->next_write = 0;
    init_spinlock(&fifo->lock);
    return;
}


PUBLIC status_t fifo_write(fifo_t *fifo, void *item)
{
    if (item == NULL)
    {
        return K_INVAILD_ADDR;
    }

    if (fifo->free == 0) /* 没有空余 */
    {
        return K_OUT_OF_RESOURCE;
    }
    spinlock_lock(&fifo->lock);
    fifo->free--;
    memcpy(
        (uint8_t *)fifo->data + fifo->item_size * fifo->next_write,
        item,
        fifo->item_size
    );
    fifo->next_write = (fifo->next_write + 1) % fifo->size;
    spinlock_unlock(&fifo->lock);
    return K_SUCCESS;
}

PUBLIC status_t fifo_read(fifo_t *fifo, void *item)
{
    if (item == NULL)
    {
        return K_INVAILD_ADDR;
    }
    if (fifo->free == fifo->size)
    {
        return K_OUT_OF_RESOURCE;
    }
    spinlock_lock(&fifo->lock);
    fifo->free++;
    memcpy(
        item,
        (uint8_t *)fifo->data + fifo->item_size * fifo->next_read,
        fifo->item_size
    );
    fifo->next_read = (fifo->next_read + 1) % fifo->size;
    spinlock_unlock(&fifo->lock);
    return K_SUCCESS;
}

PUBLIC bool fifo_empty(fifo_t *fifo)
{
    return (fifo->free == fifo->size);
}

PUBLIC bool fifo_full(fifo_t *fifo)
{
    return (fifo->free == 0);
}