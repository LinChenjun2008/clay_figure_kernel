// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/task/fork.h>
#include <asm/task/process.h>

#include <mem.h>
#include <mem/page.h>
#include <print.h> // ASSERT
#include <task.h>
#include <task/fork.h>
#include <task/schedule.h>
#include <task/struct.h>

pid_t sys_fork(void)
{
    struct task *curr = get_current_task();
    struct task *fork = allocate_task_struct();

    const char *name = curr->name;
    uint64_t    prio = curr->prio;

    if (fork == NULL)
    {
        return -1;
    }
    ASSERT(fork->kstack_base == 0 && fork->kstack_pages == 0);

    size_t ustack_pages = curr->ustack_pages;

    size_t    kstack_pages = curr->kstack_pages;
    uintptr_t kstack_base  = (uintptr_t)allocate_pages(kstack_pages);
    if (kstack_base == 0)
    {
        goto fail;
    }
    init_task_struct(fork, name, prio, kstack_base, kstack_pages, ustack_pages);

    fork->pg_dir = create_pg_dir();
    if (fork->pg_dir == NULL)
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
    atomic_inc(&curr->childs);
    cpu_task_enqueue(fork);
    return fork->pid;

fail:
    destory_mm_struct(fork->mm);
    free_pg_table(fork->pg_dir);
    free_pages((void *)fork->kstack_base, fork->kstack_pages);
    destory_task_struct(fork);
    return -1;
}