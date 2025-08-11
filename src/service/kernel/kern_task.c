// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <kernel/syscall.h>
#include <service.h>
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
    int in_status = (int)msg->m[IN_KERN_EXIT_STATUS];

    proc_exit(in_status);
    while (1);

    // Never return
    return SYSCALL_ERROR;
}

PUBLIC syscall_status_t kern_get_pid(message_t *msg)
{
    pid_t *out_pid = (pid_t *)&msg->m[OUT_KERN_GET_PID_PID];

    task_struct_t *task = running_task();

    *out_pid = task->pid; // or msg->src

    return SYSCALL_SUCCESS;
}

PUBLIC syscall_status_t kern_get_ppid(message_t *msg)
{
    pid_t *out_ppid = (pid_t *)&msg->m[OUT_KERN_GET_PPID_PPID];

    task_struct_t *task = running_task();

    *out_ppid = task->ppid;

    return SYSCALL_SUCCESS;
}

PUBLIC syscall_status_t kern_create_proc(message_t *msg)
{
    char *in_name = (char *)msg->m[IN_KERN_CREATE_PROC_NAME];
    void *in_proc = (void *)msg->m[IN_KERN_CREATE_PROC_PROC];

    pid_t *out_pid = (pid_t *)&msg->m[OUT_KERN_CREATE_PROC_PID];

    task_struct_t *task = running_task();

    char name[32];
    /// TODO: 验证地址
    memcpy(name, in_name, 32);
    name[31] = '\0';
    task_struct_t *new_task;
    new_task = proc_execute(name, task->priority, task->kstack_size, in_proc);

    *out_pid = new_task->pid;
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
    pid_t in_pid = (pid_t)msg->m[IN_KERN_WAITPID_PID];
    int   in_opt = (int)msg->m[IN_KERN_WAITPID_OPT];

    int   *out_status = (int *)&msg->m[OUT_KERN_WAITPID_STATUS];
    pid_t *out_pid    = (pid_t *)&msg->m[OUT_KERN_WAITPID_PID];

    task_struct_t *task = running_task();

    if (atomic_read(&task->childs) == 0)
    {
        return SYSCALL_ERROR;
    }

    if (!task_has_exited_child(task->pid) && (in_opt & WNOHANG))
    {
        PR_LOG(LOG_DEBUG, "in opt: %d.\n", in_opt);
        while (1);

        return K_ERROR;
    }
    // 用while防止意外唤醒(但是这种情况不应该发生)
    while (!task_has_exited_child(task->pid))
    {
        task_block(TASK_WAITING);
    }

    task_struct_t *child;
    list_node_t   *child_node;

    // any task
    if (in_pid == -1)
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

    child = CONTAINER_OF(task_struct_t, general_tag, child_node);

    *out_pid    = child->pid;
    *out_status = task_release_resource(child->pid);

    return SYSCALL_SUCCESS;
}