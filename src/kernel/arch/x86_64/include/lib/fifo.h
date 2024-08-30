/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#ifndef __FIFO_H__
#define __FIFO_H__

typedef struct fifo_s
{
    union
    {
        uint8_t  *buf8;
        uint16_t *buf16;
        uint32_t *buf32;
        uint64_t *buf64;
    };
    int type; /* 类型(8,16,32或64) */
    int size; /* 大小(最大元素数) */
    int free;
    int nr;
    int nw;
} fifo_t;

PUBLIC void init_fifo(fifo_t *fifo,void* buf,int type,int size);
PUBLIC status_t fifo_put(fifo_t *fifo,void* data);

PUBLIC status_t fifo_get(fifo_t *fifo,void* data);
PUBLIC bool fifo_empty(fifo_t *fifo);
PUBLIC bool fifo_fill(fifo_t *fifo);

#endif
