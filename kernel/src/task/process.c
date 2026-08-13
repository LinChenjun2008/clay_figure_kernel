// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
#include <asm/page.h>
#include <asm/ptrace.h>
#include <asm/task.h>

#include <print.h>
#include <std/string.h>
#include <task.h>
#include <task/struct.h>

static void kernel_process(void *func)
{
    intr_disable();

    struct task *task = get_current_task();

    task->ustack_base = (uintptr_t)allocate_pages(task->ustack_pages);
    ASSERT(task->ustack_base != 0);

    ASSERT(task->pg_dir != NULL);
    size_t    ustack_size  = task->ustack_pages * PG_SIZE;
    uint64_t *pg_dir       = task->pg_dir;
    void     *ustack       = VIRT_TO_PHYS(task->ustack_base);
    void     *ustack_vaddr = (void *)(USER_STACK_VADDR_TOP - ustack_size);
    page_map(pg_dir, ustack, ustack_vaddr, task->ustack_pages);
    task_pg_active(task);

    switch_to_user(func);

    while (1);
}

static void *create_pg_dir(void)
{
    uint64_t *pg_dir = NULL;
    pg_dir           = allocate_pages(1);
    if (pg_dir == 0)
    {
        return NULL;
    }
    struct task_mgr *task_mgr = get_task_mgr();

    uint64_t *kernel_pg_dir = PHYS_TO_VIRT(task_mgr->kernel_page_table_pos);
    memset(pg_dir, 0, PT_SIZE);
    memcpy(pg_dir + 0x100, kernel_pg_dir + 0x100, PT_SIZE / 2);
    return VIRT_TO_PHYS(pg_dir);
}

struct task *process_execute(
    const char *name,
    uint64_t    prio,
    size_t      kstack_pages,
    size_t      ustack_pages,
    void       *func
)
{
    ASSERT(name != NULL);
    ASSERT(kstack_pages != 0);
    ASSERT(ustack_pages != 0);
    ASSERT(func != 0);

    struct task *task = allocate_task();
    if (task == NULL)
    {
        return NULL;
    }

    uintptr_t kstack_base = (uintptr_t)allocate_pages(kstack_pages);
    if (kstack_base == 0)
    {
        free_task(task);
        return NULL;
    }
    init_task_struct(task, name, prio, kstack_base, kstack_pages, ustack_pages);
    create_task_context(task, kernel_process, func);

    task->pg_dir = create_pg_dir();
    if (task->pg_dir == NULL)
    {
        free_pages((void *)kstack_base, kstack_pages);
        free_task(task);
        return NULL;
    }

    get_current_task()->childs++;

    struct cpu *cpu = get_cpu_struct(task->cpu_id);

    cpu_task_list_insert(cpu, task);
    return task;
}