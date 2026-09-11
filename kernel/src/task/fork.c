// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
#include <asm/task/fork.h>
#include <asm/task/process.h>

#include <mem.h>
#include <mem/page.h>
#include <panic.h>
#include <task.h>
#include <task/fork.h>
#include <task/schedule.h>
#include <task/struct.h>

pid_t process_fork(void)
{
    ASSERT(intr_get_status() == INTR_OFF);

    struct task *curr = get_current_task();
    struct task *fork = allocate_task_struct();

    const char *name = curr->name;
    uint64_t    prio = curr->prio;

    if (fork == NULL)
    {
        return -1;
    }
    fork->kstack_base  = 0;
    fork->kstack_pages = 0;

    size_t ustack_pages = curr->ustack_pages;

    size_t    kstack_pages = curr->kstack_pages;
    uintptr_t kstack_base  = (uintptr_t)allocate_pages(kstack_pages);
    if (kstack_base == 0)
    {
        goto fail;
    }
    init_task_struct(fork, name, prio, kstack_base, kstack_pages, ustack_pages);

    fork->pg_dir = create_pg_dir();
    if (fork->pg_dir == 0)
    {
        goto fail;
    }

    fork->mm = allocate_mm_struct();
    if (fork->mm == NULL)
    {
        goto fail;
    }
    if (copy_process(fork, curr) < 0)
    {
        goto fail;
    }

    fork->pid = allocate_pid();
    if (fork->pid == -1)
    {
        goto fail;
    }
    fork->ppid = curr->pid;
    if (pid_table_insert(fork) < 0)
    {
        release_pid(fork->pid);
        goto fail;
    }

    spin_lock(&curr->childs_lock);
    list_append(&curr->childs_list, &fork->parent_node);
    spin_unlock(&curr->childs_lock);

    cpu_task_enqueue(fork);
    return fork->pid;

fail:
    destory_mm_struct(fork->mm);
    free_pg_table(fork->pg_dir);
    free_pages((void *)fork->kstack_base, fork->kstack_pages);
    destory_task_struct(fork);
    return -1;
}