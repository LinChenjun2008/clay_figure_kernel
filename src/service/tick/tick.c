// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <kernel/syscall.h> // send_recv
#include <service.h>        // message type
#include <std/string.h>     // memset

PUBLIC void tick_main(void)
{
    uint64_t  ticks = 0;
    message_t msg;
    while (1)
    {
        memset(&msg, 0, sizeof(msg));
        send_recv(NR_RECV, RECV_FROM_ANY, &msg);
        switch (msg.type)
        {
            case RECV_FROM_INT:
                ticks++;
                break;
            case TICK_GET_TICKS:
                msg.m[OUT_TICK_GET_TICKS_TICKS] = ticks;
                send_recv(NR_SEND, msg.src, &msg);
                break;
            default:
                break;
        }
    }
}