/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

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
    uint32_t pixel_per_scanline;
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
            *( gi.vram + (msg->m3.i4 + y) * gi.pixel_per_scanline
              + msg->m3.i3 + x) = pixel;
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
    gi.pixel_per_scanline = msg.m3.i3;
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