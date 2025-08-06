// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <kernel/syscall.h>
#include <service.h>

typedef syscall_status_t (*kern_syscall_t)(message_t *);

// previous prototype for each function

// kern_task.c
PUBLIC syscall_status_t kern_exit(message_t *msg);
PUBLIC syscall_status_t kern_get_pid(message_t *msg);
PUBLIC syscall_status_t kern_get_ppid(message_t *msg);

// kern_mem.c
PUBLIC syscall_status_t kern_allocate_page(message_t *msg);
PUBLIC syscall_status_t kern_free_page(message_t *msg);
PUBLIC syscall_status_t kern_read_task_mem(message_t *msg);

PRIVATE kern_syscall_t kern_syscalls[KERN_SYSCALLS] = {
    kern_exit, // exit
    kern_get_pid,
    kern_get_ppid,
    NULL, // create_proc
    NULL, // waitpid
    kern_allocate_page,
    kern_free_page,
    kern_read_task_mem,
};

PUBLIC syscall_status_t kernel_services(message_t *msg)
{
    syscall_status_t ret = SYSCALL_ERROR;
    if (msg->type < KERN_SYSCALLS)
    {
        if (kern_syscalls[msg->type] != NULL)
        {
            ret = kern_syscalls[msg->type](msg);
        }
    }
    return ret;
}
