// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <kernel/syscall.h>
#include <mem/page.h> // PG_SIZE
#include <service.h>
#include <std/string.h> // memset
#include <ulib.h>

PRIVATE struct
{
    uint32_t *vram;
    uint32_t  xsize;
    uint32_t  ysize;
    uint32_t  pixel_per_scanline;
} gi;

PRIVATE void view_fill(message_t *msg)
{

    void    *in_buffer      = (void *)msg->m[IN_VIEW_FILL_BUFFER];
    size_t   in_buffer_size = (size_t)msg->m[IN_VIER_FILL_BUFFER_SIZE];
    uint32_t in_xsize       = (uint32_t)msg->m[IN_VIEW_FILL_XSIZE];
    uint32_t in_ysize       = (uint32_t)msg->m[IN_VIEW_FILL_YSIZE];
    uint32_t in_x           = (uint32_t)msg->m[IN_VIEW_FILL_X];
    uint32_t in_y           = (uint32_t)msg->m[IN_VIEW_FILL_Y];

    // alloc buffer
    uint32_t *buf = allocate_page();
    if (buf == NULL)
    {
        return;
    }

    // read buffer
    read_task_addr(msg->src, in_buffer, in_buffer_size, buf);

    // print buf to screen.
    uint32_t x, y;
    for (y = 0; y < in_ysize; y++)
    {
        for (x = 0; x < in_xsize; x++)
        {
            uint32_t pixel = *(buf + y * in_xsize + x);
            *(gi.vram + (in_y + y) * gi.pixel_per_scanline + in_x + x) = pixel;
        }
    }
    free_page(buf);
    return;
}

PUBLIC void view_main()
{
    graph_info_t *g_graph_info = &BOOT_INFO->graph_info;
    gi.vram                    = (uint32_t *)g_graph_info->frame_buffer_base;
    gi.xsize                   = g_graph_info->horizontal_resolution;
    gi.ysize                   = g_graph_info->vertical_resolution;
    gi.pixel_per_scanline      = g_graph_info->pixel_per_scanline;
    message_t msg;
    while (1)
    {
        memset(&msg, 0, sizeof(msg));
        send_recv(NR_RECV, RECV_FROM_ANY, &msg);
        switch (msg.type)
        {
            case VIEW_FILL:
                view_fill(&msg);
                send_recv(NR_SEND, msg.src, &msg);
                break;
            default:
                break;
        }
    }
}
