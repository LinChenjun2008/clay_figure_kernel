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