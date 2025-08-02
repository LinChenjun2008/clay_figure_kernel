// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <service.h>
#include <task/task.h>

PRIVATE pid_t service_pid_table[SERVICES];

PUBLIC bool is_service_id(uint32_t sid)
{
    return sid >= SERVICE_ID_BASE && sid < SERVICE_ID_BASE + SERVICES;
}

PUBLIC pid_t service_id_to_pid(uint32_t sid)
{
    if (is_service_id(sid))
    {
        return service_pid_table[sid - SERVICE_ID_BASE];
    }
    return PID_NO_TASK;
}

PUBLIC void service_init(void)
{
    service_pid_table[TICK - SERVICE_ID_BASE] =
        prog_execute("TICK", SERVICE_PRIORITY, 4096, tick_main)->pid;
    service_pid_table[MM - SERVICE_ID_BASE] =
        task_start("MM", SERVICE_PRIORITY, 4096, mm_main, 0)->pid;
    service_pid_table[VIEW - SERVICE_ID_BASE] =
        prog_execute("VIEW", SERVICE_PRIORITY, 4096, view_main)->pid;
    service_pid_table[USB_SRV - SERVICE_ID_BASE] =
        task_start("USB service", SERVICE_PRIORITY, 4096, usb_main, 0)->pid;
    service_pid_table[KBD_SRV - SERVICE_ID_BASE] =
        task_start("Keyboard service", SERVICE_PRIORITY, 4096, keyboard_main, 0)
            ->pid;
    return;
}