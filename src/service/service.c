// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <kernel/syscall.h> // message_t
#include <service.h>
#include <task/task.h>

PRIVATE struct
{
    uint32_t    kernel_task;
    pid_t       service_id;
    const char *name;
    size_t      kstack_size;
    void       *func;
} services[SERVICES] = {
    { 0, TICK, "TICK", 4096, tick_main },
    { 1, MM, "MM", 4096, mm_main },
    { 0, VIEW, "VIEW", 4096, view_main },
    { 1, USB_SRV, "USB Service", 4096, usb_main },
    { 1, KBD_SRV, "Keyboard Services", 4096, keyboard_main },
};

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
    int i;
    for (i = 0; i < SERVICES; i++)
    {
        task_struct_t *task;
        const char    *name        = services[i].name;
        size_t         kstack_size = services[i].kstack_size;
        void          *func        = services[i].func;
        if (services[i].kernel_task)
        {
            task = task_start(name, SERVICE_PRIORITY, kstack_size, func, 0);
        }
        else
        {
            task = prog_execute(name, SERVICE_PRIORITY, kstack_size, func);
        }

        int index = services[i].service_id - SERVICE_ID_BASE;

        service_pid_table[index] = task->pid;
    }
    return;
}
