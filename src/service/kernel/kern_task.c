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
PUBLIC syscall_status_t kern_waitpid(message_t *msg);

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
    msg3_t        *m3   = &msg->m3;
    task_struct_t *task = running_task();

    char name[32];
    /// TODO: 验证地址
    memcpy(name, m3->p1, 32);
    name[31] = '\0';
    task_struct_t *new_task;
    new_task = proc_execute(name, task->priority, task->kstack_size, m3->p2);

    msg1_t *m1 = &msg->m1;
    m1->i1     = new_task->pid;
    return SYSCALL_SUCCESS;
}

PRIVATE int find_child(list_node_t *node, uint64_t arg)
{
    task_struct_t *child;
    child = CONTAINER_OF(task_struct_t, general_tag, node);
    if (child->pid == (pid_t)arg)
    {
        return TRUE;
    }
    return FALSE;
}

PUBLIC syscall_status_t kern_waitpid(message_t *msg)
{
    // in:
    // m1.i1 = pid

    // out
    // m1.i1 = return value
    // m1.i2 = exited child pid

    msg1_t        *m1   = &msg->m1;
    task_struct_t *task = running_task();

    m1->i2 = 0;
    if (atomic_read(&task->childs) == 0)
    {
        return SYSCALL_ERROR;
    }

    // 用while防止意外唤醒(但是这种情况不应该发生)
    while (!task_has_exited_child(task->pid))
    {
        task_block(TASK_WAITING);
    }

    pid_t          pid = m1->i1;
    task_struct_t *child;
    list_node_t   *child_node;
    int            ret_val;
    if (pid == PID_NO_TASK)
    {
        spinlock_lock(&task->child_list_lock);
        child_node = list_pop(&task->exited_child_list);
        spinlock_unlock(&task->child_list_lock);
    }
    else
    {
        spinlock_lock(&task->child_list_lock);
        child_node =
            list_traversal(&task->exited_child_list, find_child, task->pid);
        spinlock_unlock(&task->child_list_lock);
        if (child_node == NULL)
        {
            return SYSCALL_SUCCESS;
        }
    }
    child   = CONTAINER_OF(task_struct_t, general_tag, child_node);
    pid     = child->pid;
    ret_val = task_release_resource(child->pid);
    m1->i1  = ret_val;
    m1->i2  = pid;
    return SYSCALL_SUCCESS;
}