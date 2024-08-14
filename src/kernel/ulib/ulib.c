#include <kernel/global.h>
#include <service.h>
#include <kernel/syscall.h>

PUBLIC uint64_t get_ticks()
{
    message_t msg;
    msg.type = TICK_GET_TICKS;
    send_recv(NR_BOTH,TICK,&msg);
    return msg.m3.l1;
}

PUBLIC void* allocate_page(uint32_t count)
{
    message_t msg;
    msg.type = MM_ALLOCATE_PAGE;
    msg.m1.i1 = count;
    send_recv(NR_BOTH,MM,&msg);
    return msg.m2.p1;
}

PUBLIC void free_page(void *addr,uint32_t count)
{
    message_t msg;
    msg.type = MM_FREE_PAGE;
    msg.m3.p1 = addr;
    msg.m3.i1 = count;
    send_recv(NR_SEND,MM,&msg);
    return;
}

PUBLIC void read_prog_addr(pid_t pid,void *addr,size_t size,void *buffer)
{
    message_t msg;
    msg.type  = MM_READ_PROG_ADDR;
    msg.m3.i1 = pid;
    msg.m3.p1 = addr;
    msg.m3.l1 = size;
    msg.m3.p2 = buffer;
    send_recv(NR_BOTH,MM,&msg);
}

PUBLIC void put_pixel(uint32_t x,uint32_t y,uint32_t color)
{
    message_t msg;
    msg.type = VIEW_PUT_PIXEL;
    msg.m1.i1 = color;
    msg.m1.i2 = x;
    msg.m1.i3 = y;
    send_recv(NR_SEND,VIEW,&msg);
    return;
}

PUBLIC void fill(
    void *buffer,
    size_t buffer_size,
    uint32_t xsize,
    uint32_t ysize,
    uint32_t x,
    uint32_t y)
{
    message_t msg;
    msg.type = VIEW_FILL;
    msg.m3.p1 = buffer;
    msg.m3.l1 = buffer_size;
    msg.m3.i1 = xsize;
    msg.m3.i2 = ysize;

    msg.m3.i3 = x;
    msg.m3.i4 = y;
    send_recv(NR_BOTH,VIEW,&msg);
    return;
}