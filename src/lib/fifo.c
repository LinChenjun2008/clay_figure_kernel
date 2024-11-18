/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU Lesser General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 宽通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 宽通用公共许可证，了解详情。

你应该随程序获得一份 GNU 宽通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

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
        return K_ERROR;
    }

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
    return K_SUCCESS;
}

PUBLIC status_t fifo_get(fifo_t *fifo,void* data)
{
    if (data == NULL)
    {
        return K_ERROR;
    }
    if (fifo->free == fifo->size)
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