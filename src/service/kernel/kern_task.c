// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <kernel/syscall.h>
#include <std/string.h> // memcpy
#include <task/task.h>  // running_task

// previous prototype for each function
PUBLIC syscall_status_t kern_exit(message_t *msg);
PUBLIC syscall_status_t kern_get_pid(message_t *msg);
PUBLIC syscall_status_t kern_get_ppid(message_t *msg);
PUBLIC syscall_status_t kern_create_proc(message_t *msg);

PUBLIC syscall_status_t kern_exit(message_t *msg)
{
    // in:
    // m1.i1 = return value

    msg1_t *m1 = &msg->m1;

    proc_exit(m1->i1);
    while (1);

    // Never return
    return SYSCALL_ERROR;
}

PUBLIC syscall_status_t kern_get_pid(message_t *msg)
{
    // out:
    // m1.i1 = current pid

    msg1_t        *m1   = &msg->m1;
    task_struct_t *task = running_task();

    m1->i1 = task->pid; // or msg->src

    return SYSCALL_SUCCESS;
}

PUBLIC syscall_status_t kern_get_ppid(message_t *msg)
{
    // out
    // m1.i1 = ppid

    msg1_t        *m1   = &msg->m1;
    task_struct_t *task = running_task();

    m1->i1 = task->ppid;

    return SYSCALL_SUCCESS;
}

PUBLIC syscall_status_t kern_create_proc(message_t *msg)
{
    task_struct_t *task = running_task();

    msg3_t *m3 = &msg->m3;
    char    name[32];
    /// TODO: 验证地址
    memcpy(name, m3->p1, 32);
    name[31] = '\0';
    task_struct_t *new_task;
    new_task = proc_execute(name, task->priority, task->kstack_size, m3->p2);

    msg1_t *m1 = &msg->m1;
    m1->i1     = new_task->pid;
    return SYSCALL_SUCCESS;
}