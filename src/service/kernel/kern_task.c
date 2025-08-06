// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <kernel/syscall.h>
#include <task/task.h> // running_task

// previous prototype for each function
PUBLIC syscall_status_t kern_exit(message_t *msg);
PUBLIC syscall_status_t kern_get_pid(message_t *msg);

PUBLIC syscall_status_t kern_exit(message_t *msg)
{
    // in:
    // m1.i1 = return value
    msg1_t *m1 = &msg->m1;

    proc_exit(m1->i1);
    while (1);

    // Never return
    return K_ERROR;
}

PUBLIC syscall_status_t kern_get_pid(message_t *msg)
{
    // out:
    // m1.i1 = current pid

    msg1_t *m1 = &msg->m1;
    m1->i1     = msg->src;

    return SYSCALL_SUCCESS;
}