// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <mem/page.h>
#include <print.h>
#include <syscall/ipc.h>
#include <sysinfo.h>
#include <task.h>
#include <task/schedule.h>
#include <task/wait.h>

static void main_adopt_childs(struct task *task)
{
    struct task *main_task = get_cpu_struct(task->cpu_id)->main_task;
    if (main_task == NULL || main_task == task)
    {
        return;
    }
    struct task_mgr *task_mgr = get_task_mgr();

    spin_lock(&task_mgr->lock);
    int i;
    for (i = 0; i < task_mgr->max_tasks; i++)
    {
        struct task *child = task_mgr->task_table[i];
        if (child == NULL)
        {
            continue;
        }
        if (child->ppid != task->pid)
        {
            continue;
        }
        child->ppid = main_task->pid;
    }
    spin_unlock(&task_mgr->lock);

    spin_lock_double(&task->exited_lock, &main_task->exited_lock);
    while (!list_empty(&task->exited_childs))
    {
        struct list_node *node  = list_pop(&task->exited_childs);
        struct task      *child = CONTAINER_OF(struct task, general_node, node);
        child->ppid             = main_task->pid;

        list_append(&main_task->exited_childs, node);
    }
    spin_unlock_double(&task->exited_lock, &main_task->exited_lock);

    atomic_add(&main_task->childs, atomic_read(&task->childs));
    atomic_set(&task->childs, 0);
    return;
}

void task_exit(int return_value)
{
    struct task *task = get_current_task();

    ASSERT(task->preempt_count == 0);

    task->return_status = return_value;

    main_adopt_childs(task);

    mailbox_cleanup(task);

    task_block(TASK_DIED);
    return;
}

static size_t exited_childs(struct task *task)
{
    spin_lock(&task->exited_lock);
    size_t exited_childs = list_len(&task->exited_childs);
    spin_unlock(&task->exited_lock);
    return exited_childs;
}

static int find_child(struct list_node *node, void *arg)
{
    struct task *task = NULL;
    task              = CONTAINER_OF(struct task, general_node, node);
    return task->pid == *(pid_t *)arg;
}

int task_release_resources(struct task *task)
{
    struct task *parent_task = pid_to_task(task->ppid);
    ASSERT(get_current_task() == parent_task);

    free_pages((void *)task->kstack_base, task->kstack_pages);
    int ret = task->return_status;
    atomic_dec(&parent_task->childs);
    destory_task_struct(task);
    return ret;
}

pid_t task_waitpid(pid_t pid, int *status, int options)
{
    // unsupport
    if (pid < -1)
    {
        return -1;
    }
    if (pid != PID_ANY && !check_pid_avaiability(pid))
    {
        return -1;
    }
    struct task *task = get_current_task();

    if (atomic_read(&task->childs) == 0)
    {
        return -1;
    }
    if (exited_childs(task) == 0 && options & WNOHANG)
    {
        return 0;
    }

    while (exited_childs(task) == 0)
    {
        task_block(TASK_WAITING);
    }

    struct list_node *node;

    // any task
    if (pid == PID_ANY)
    {
        spin_lock(&task->exited_lock);
        node = list_pop(&task->exited_childs);
        spin_unlock(&task->exited_lock);
    }
    else
    {
        spin_lock(&task->exited_lock);
        node = list_traversal_remove(&task->exited_childs, find_child, &pid);
        spin_unlock(&task->exited_lock);

        if (node == NULL)
        {
            return -1;
        }
    }

    struct task *child      = CONTAINER_OF(struct task, general_node, node);
    pid_t        ret        = child->pid;
    int          ret_status = task_release_resources(child);
    if (status != NULL)
    {
        *status = ret_status;
    }
    return ret;
}