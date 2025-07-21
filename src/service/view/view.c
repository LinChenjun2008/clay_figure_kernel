// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <kernel/syscall.h>
#include <mem/page.h> // PG_SIZE
#include <service.h>
#include <ulib.h>

PRIVATE struct
{
    uint32_t *vram;
    uint32_t  xsize;
    uint32_t  ysize;
    uint32_t  pixel_per_scanline;
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
    read_prog_addr(msg->src, msg->m3.p1, msg->m3.l1, buf);

    // print buf to screen.
    uint32_t x, y;
    for (y = 0; y < msg->m3.i2; y++)
    {
        for (x = 0; x < msg->m3.i1; x++)
        {
            uint32_t pixel = *(buf + y * msg->m3.i1 + x);
            *(gi.vram + (msg->m3.i4 + y) * gi.pixel_per_scanline + msg->m3.i3 +
              x)           = pixel;
        }
    }
    free_page(buf, msg->m3.l1 / PG_SIZE + 1);
    return;
}

PUBLIC void view_main()
{
    gi.vram               = (uint32_t *)g_graph_info->frame_buffer_base;
    gi.xsize              = g_graph_info->horizontal_resolution;
    gi.ysize              = g_graph_info->vertical_resolution;
    gi.pixel_per_scanline = g_graph_info->pixel_per_scanline;
    message_t msg;
    while (1)
    {
        send_recv(NR_RECV, RECV_FROM_ANY, &msg);
        switch (msg.type)
        {
            case VIEW_PUT_PIXEL:
                view_put_pixel(&msg);
                break;
            case VIEW_FILL:
                view_fill(&msg);
                send_recv(NR_SEND, msg.src, &msg);
                break;
            default:
                break;
        }
    }
}
