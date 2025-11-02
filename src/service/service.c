// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 Lin Chenjun
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
    size_t      kstack_pages;
    size_t      ustack_pages;
    void       *func;
} services[SERVICES] = {
    { 0, TICK, "TICK", 1, 1, tick_main },
    { 1, MM, "MM", 1, 1, mm_main },
    { 0, VIEW, "VIEW", 1, 1, view_main },
    { 1, USB_SRV, "USB Service", 1, 1, usb_main },
    { 1, KBD_SRV, "Keyboard Services", 1, 1, keyboard_main },
};

PRIVATE pid_t service_pid_table[SERVICES] = { PID_NO_TASK };

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
    int      i;
    uint64_t prio = SERVICE_PRIORITY;
    for (i = 0; i < SERVICES; i++)
    {
        task_struct_t *task;
        const char    *name         = services[i].name;
        size_t         kstack_pages = services[i].kstack_pages;
        size_t         ustack_pages = services[i].ustack_pages;
        void          *func         = services[i].func;
        if (services[i].kernel_task)
        {
            task = task_start(name, prio, kstack_pages, func, 0);
        }
        else
        {
            task = proc_execute(name, prio, kstack_pages, ustack_pages, func);
        }

        int index = services[i].service_id - SERVICE_ID_BASE;

        service_pid_table[index] = task->pid;
    }
    return;
}
