// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <kernel/syscall.h> // sys_send_recv
#include <service.h>        // message type

PUBLIC void mm_main()
{
    message_t msg;
    while (1)
    {
        sys_send_recv(NR_RECV, RECV_FROM_ANY, &msg);
        switch (msg.type)
        {
            default:
                break;
        }
    }
}