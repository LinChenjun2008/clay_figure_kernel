/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <kernel/syscall.h> // send_recv
#include <service.h>        // message type

#define MAX_TIMERS 512

#define TIMER_UNUSED       0
#define TIMER_WAIT_TIMEOUT 1

typedef struct
{
    uint64_t timeout;
    uint32_t flag;
    pid_t    holder;
} timer_t;

typedef struct
{
    uint64_t ticks;
    uint64_t using_timers;
    timer_t  timers[MAX_TIMERS];
} timer_manager_t;

PRIVATE timer_manager_t time_manager;

PRIVATE status_t add_timer(message_t *msg)
{
    int i;
    for (i = 0;i < MAX_TIMERS;i++)
    {
        if (time_manager.timers[i].flag == TIMER_UNUSED)
        {
            time_manager.using_timers++;
            time_manager.timers[i].timeout = time_manager.ticks + msg->m3.l1;
            time_manager.timers[i].flag    = TIMER_WAIT_TIMEOUT;
            time_manager.timers[i].holder  = msg->src;
            return K_SUCCESS;
        }
    }
    return K_ERROR;
}

PRIVATE void wake_up()
{
    int i;
    for (i = 0;i < MAX_TIMERS;i++)
    {
        if (time_manager.timers[i].flag != TIMER_UNUSED
            && time_manager.ticks >= time_manager.timers[i].timeout)
        {
            message_t msg;
            msg.m3.l1 = 0;
            send_recv(NR_SEND,time_manager.timers[i].holder,&msg);
            time_manager.using_timers--;
            time_manager.timers[i].flag = TIMER_UNUSED;
        }
    }
}

PUBLIC void tick_main()
{
    time_manager.ticks        = 0;
    time_manager.using_timers = 0;
    int i;
    for (i = 0;i < MAX_TIMERS;i++)
    {
        time_manager.timers[i].flag = TIMER_UNUSED;
    }
    message_t msg;
    while(1)
    {
        send_recv(NR_RECV,RECV_FROM_ANY,&msg);
        switch (msg.type)
        {
            case RECV_FROM_INT:
                time_manager.ticks += msg.m1.i1;
                wake_up();
                break;
            case TICK_GET_TICKS:
                msg.m3.l1 = time_manager.ticks;
                send_recv(NR_SEND,msg.src,&msg);
                break;
            case TICK_SLEEP:
                add_timer(&msg);
                break;
            default:
                break;
        }
    }
}