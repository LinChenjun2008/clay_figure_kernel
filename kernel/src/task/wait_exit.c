// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <mem/page.h>
#include <panic.h>
#include <syscall/ipc.h>
#include <sysinfo.h>
#include <task.h>
#include <task/schedule.h>
#include <task/wait.h>

void init_adopt_childs(struct task *task)
{
    struct task *init_task = pid_to_task(1);
    ASSERT(init_task != NULL && init_task != task);

    spin_lock_double(&task->childs_lock, &init_task->childs_lock);
    while (!list_empty(&task->childs_list))
    {
        struct list_node *node  = list_pop(&task->childs_list);
        struct task      *child = CONTAINER_OF(struct task, parent_node, node);
        child->ppid             = 1;

        list_append(&init_task->childs_list, node);
    }
    while (!list_empty(&task->exited_childs))
    {
        struct list_node *node  = list_pop(&task->exited_childs);
        struct task      *child = CONTAINER_OF(struct task, general_node, node);
        child->ppid             = 1;

        list_append(&init_task->exited_childs, node);
    }
    spin_unlock_double(&task->childs_lock, &init_task->childs_lock);

    return;
}

void task_exit(int return_value)
{
    struct task *task = get_current_task();

    ASSERT(task->preempt_count == 0);

    task->return_status = return_value;

    init_adopt_childs(task);

    mailbox_cleanup(task);

    task_block(TASK_DIED);
    return;
}

static size_t exited_childs(struct task *task)
{
    spin_lock(&task->childs_lock);
    size_t exited_childs = list_len(&task->exited_childs);
    spin_unlock(&task->childs_lock);
    return exited_childs;
}

static int find_child(struct list_node *node, void *arg)
{
    struct task *task = NULL;
    task              = CONTAINER_OF(struct task, general_node, node);
    return task->pid == *(pid_t *)arg;
}

static int task_release_resources(struct task *task)
{
    pid_table_remove(task);
    release_pid(task->pid);

    struct task *parent_task = pid_to_task(task->ppid);
    ASSERT(get_current_task() == parent_task);

    free_pages((void *)task->kstack_base, task->kstack_pages);
    int ret = task->return_status;

    spin_lock(&parent_task->childs_lock);
    list_remove(&task->parent_node);
    spin_unlock(&parent_task->childs_lock);

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

    spin_lock(&task->childs_lock);
    int childs = list_len(&task->childs_list);
    spin_unlock(&task->childs_lock);
    if (childs == 0)
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
        spin_lock(&task->childs_lock);
        node = list_pop(&task->exited_childs);
        spin_unlock(&task->childs_lock);
    }
    else
    {
        spin_lock(&task->childs_lock);
        node = list_traversal_remove(&task->exited_childs, find_child, &pid);
        spin_unlock(&task->childs_lock);

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