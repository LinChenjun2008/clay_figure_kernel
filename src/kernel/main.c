#include <kernel/global.h>
#include <common.h>
#include <kernel/init.h>
#include <intr.h>
#include <task/task.h>
#include <kernel/syscall.h>

#include <service.h>
#include <log.h>

boot_info_t *g_boot_info = (boot_info_t*)0xffff800000310000;

void ktask()
{
    uint32_t color = 0x00000000;
    while(1)
    {
        uint32_t x,y;
        for(y = 10;y < 20;y++)
        {
            uint32_t *buffer = \
                (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
                + g_boot_info->graph_info.horizontal_resolution * y;
            for (x = 0;x < 10;x++)
            {
                *(buffer + x) = color++;
            }
        }
        __asm__ ("hlt");
    };
}

extern uint64_t global_ticks;
static uint32_t buf[10][10];
void ktask2()
{
    uint32_t color = 0x00000000;
    uint64_t sec = 0;
    message_t msg;

    msg.type = TICK_GET_TICKS;
    if (send_recv(NR_BOTH,TICK,&msg) != SYSCALL_SUCCESS)
    {
        pr_log("\3get tick failed.\n");
    }
    sec = msg.m3.l1 / 100;
    while(1)
    {
        msg.type = TICK_GET_TICKS;
        if (send_recv(NR_BOTH,TICK,&msg) != SYSCALL_SUCCESS)
        {
            pr_log("\3get tick failed.\n");
        }
        if (sec + 60 <= msg.m3.l1 / 100)
        {
            sec = msg.m3.l1 / 100;
            pr_log("\1[ %d min]\n",sec / 60);
        }
        if (msg.m3.l1 % 25 <= 5)
        {
            msg.type = VIEW_FILL;
            msg.m3.p1 = buf;
            msg.m3.l1 = sizeof(buf);
            msg.m3.i1 = 10;
            msg.m3.i2 = 10;

            msg.m3.i3 = 0;
            msg.m3.i4 = 20;
            send_recv(NR_BOTH,VIEW,&msg);
        }
        uint32_t x,y;
        for(y = 0;y < 10;y++)
        {
            for (x = 0;x < 10;x++)
            {
                // msg.m1.i1 = color++;
                // msg.m1.i2 = x;
                // msg.m1.i3 = y;
                // msg.type =  VIEW_PUT_PIXEL;
                // send_recv(NR_SEND,VIEW,&msg);

                // *(buffer + x) = color++;
                buf[y][x] = color++;
            }
        }
    };
}

void kernel_main()
{
    pr_log("\1Kernel initializing.\n");
    init_all();
    pr_log("\1Kernel initializing done.\n");

    task_start("k task",DEFAULT_PRIORITY,65536,ktask,0);
    prog_execute(ktask2,"k task 2",65536);
    uint32_t color = 0;
    while(1)
    {
        uint32_t x,y;
        for(y = 0;y < 10;y++)
        {
            uint32_t *buffer = \
                (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
                + g_boot_info->graph_info.horizontal_resolution * y;
            for (x = 0;x < 10;x++)
            {
                *(buffer + x) = color++;
            }
        }
        __asm__ ("hlt");
    };
}