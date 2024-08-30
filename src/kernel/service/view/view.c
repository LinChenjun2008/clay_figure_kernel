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

#include <kernel/global.h>
#include <kernel/syscall.h>
#include <service.h>
#include <ulib.h>

#include <log.h>

PRIVATE struct
{
    uint32_t *vram;
    uint32_t xsize;
    uint32_t ysize;
} gi;

PRIVATE void view_put_pixel(message_t *msg)
{
    *(gi.vram + msg->m1.i3 * gi.xsize + msg->m1.i2) = msg->m1.i1;
}

PRIVATE void view_fill(message_t *msg)
{
    // alloc buffer
    uint32_t *buf = allocate_page(msg->m3.l1 / PG_SIZE + 1);
    if (buf == NULL)
    {
        return;
    }

    // read buffer
    read_prog_addr(msg->src,msg->m3.p1,msg->m3.l1,buf);

    // print buf to screen.
    uint32_t x,y;
    for (y = 0;y < msg->m3.i2;y++)
    {
        for (x = 0;x < msg->m3.i1;x++)
        {
            uint32_t pixel = *(buf + y * msg->m3.i1 + x);
            *(gi.vram + (msg->m3.i4 + y) * gi.xsize + msg->m3.i3 + x) = pixel;
        }
    }
    free_page(buf,msg->m3.l1 / PG_SIZE + 1);
    return;
}

PUBLIC void view_main()
{
    message_t msg;
    // receive param
    send_recv(NR_RECV,0,&msg);
    gi.vram  = msg.m3.p1;
    gi.xsize = msg.m3.i1;
    gi.ysize = msg.m3.i2;
    while(1)
    {
        send_recv(NR_RECV,RECV_FROM_ANY,&msg);
        switch(msg.type)
        {
            case VIEW_PUT_PIXEL:
                view_put_pixel(&msg);
                break;
            case VIEW_FILL:
                view_fill(&msg);
                send_recv(NR_SEND,msg.src,&msg);
                break;
            default:
                break;
        }
    }
}