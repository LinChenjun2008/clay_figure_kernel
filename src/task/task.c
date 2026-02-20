// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic.h>
#include <asm/mem/page.h>
#include <asm/sync/spinlock.h>
#include <asm/task.h>

#include <lib/list.h>
#include <mem/allocator.h>
#include <print.h>
#include <std/string.h>
#include <task.h>

static task_man_t *task_man;

static void cpu_task_init(cpu_t *cpu, uint8_t id)
{
    cpu->task_man = task_man;
    cpu->id       = id;

    init_list(&cpu->task_queue);
    cpu->running_tasks = 0;

    cpu->min_vrun_time = 0;
    cpu->total_weight  = 0;
    cpu->main_task     = NULL;
    return;
}

void task_init(boot_info_t *boot_info, int max_tasks)
{
    task_struct_t **task_table = NULL;
    size_t task_table_size     = sizeof(task_man->task_table[0]) * max_tasks;
    kmalloc(task_table_size, 0, 0, (void **)&task_table);
    ASSERT(task_table != NULL);

    cpu_t *cpus      = NULL;
    int    max_cpus  = apic_max_lapic_id() + 1;
    size_t cpus_size = sizeof(task_man->cpus[0]) * max_cpus;
    kmalloc(cpus_size, 0, 0, (void **)&cpus);

    kmalloc(sizeof(*task_man), 0, 0, (void **)&task_man);
    init_spinlock(&task_man->lock);
    task_man->task_table = task_table;
    task_man->max_tasks  = max_tasks;

    task_man->cpus     = cpus;
    task_man->max_cpus = max_cpus;

    task_man->kernel_page_table_pos = boot_info->page_table_pos;

    int i;
    for (i = 0; i < task_man->max_cpus; i++)
    {
        cpu_task_init(&task_man->cpus[i], i);
    }
    printk("Max tasks: %d.\n", max_tasks);
    printk("task_table at %p.\n", task_man->task_table);

    uintptr_t stack_base = (uintptr_t)PHYS_TO_VIRT(boot_info->stack_base);
    make_main_task(stack_base, boot_info->stack_pages);
    return;
}

void make_main_task(uintptr_t stack_base, size_t stack_pages)
{
    task_struct_t *task = allocate_task_struct();
    set_current_task(task);
    init_task_struct(task, "Main", DEFAULT_PRIO, stack_base, stack_pages, 0);
    task->cpu            = &task_man->cpus[get_current_cpu_id()];
    task->status         = TASK_RUNNING;
    task->cpu->main_task = task;
    return;
}

void set_current_task(task_struct_t *task)
{
    arch_set_current_task(task);
    return;
}

task_struct_t *get_current_task(void)
{
    return arch_get_current_task();
}

uint8_t get_current_cpu_id()
{
    return arch_get_current_cpu_id();
}

task_struct_t *pid_to_task(pid_t pid)
{
    if (pid > task_man->max_tasks)
    {
        return NULL;
    }
    return task_man->task_table[pid];
}

pid_t task_to_pid(task_struct_t *task)
{
    pid_t i;
    for (i = 0; i < task_man->max_tasks; i++)
    {
        if (task_man->task_table[i] == task)
        {
            return i;
        }
    }
    return PID_NO_TASK;
}

static pid_t allocate_task_lock(void)
{
    pid_t i;
    for (i = 0; i < task_man->max_tasks; i++)
    {
        if (task_man->task_table[i] == NULL)
        {
            break;
        }
    }
    size_t task_struct_size = sizeof(*task_man->task_table[0]);
    kmalloc(task_struct_size, 0, 0, (void **)&task_man->task_table[i]);
    return i;
}

task_struct_t *allocate_task_struct(void)
{
    pid_t pid;

    spin_lock(&task_man->lock);
    pid = allocate_task_lock();
    spin_unlock(&task_man->lock);

    return pid_to_task(pid);
}

void free_task_struuct(task_struct_t *task)
{
    spin_lock(&task_man->lock);
    kfree((void **)&task_man->task_table[task->pid]);
    spin_unlock(&task_man->lock);
}

void init_task_struct(
    task_struct_t *task,
    const char    *name,
    uint64_t       prio,
    uintptr_t      kstack_base,
    size_t         kstack_pages,
    size_t         ustack_pages
)
{
    memset(task, 0, sizeof(*task));
    task->context = (task_context_t *)(kstack_base + kstack_pages * PG_SIZE);
    task->kstack_base  = kstack_base;
    task->kstack_pages = kstack_pages;

    task->ustack_base  = 0;
    task->ustack_pages = ustack_pages;


    task->cpu = get_current_task()->cpu;

    task->pid  = task_to_pid(task);
    task->ppid = get_current_task()->pid;

    task->childs = 0;

    strncpy(task->name, name, 31);
    task->name[31] = '\0';

    task->status        = TASK_READY;
    task->preempt_count = 0;
    task->page_dir      = NULL;

    task->prio      = prio;
    task->run_time  = 0;
    task->vrun_time = 0;
    return;
}

task_struct_t *task_start(
    const char *name,
    uint64_t    prio,
    size_t      kstack_pages,
    void       *func,
    void       *arg
)
{
    if (!kstack_pages)
    {
        return NULL;
    }
    task_struct_t *task = allocate_task_struct();
    if (task == NULL)
    {
        return NULL;
    }
    uintptr_t kstack_base = 0;

    allocate_pages(kstack_pages, (void **)&kstack_base);
    init_task_struct(task, name, prio, kstack_base, kstack_pages, 0);
    create_task_context(task, func, arg);

    get_current_task()->childs++;
    cpu_task_list_insert(task->cpu, task);
    return task;
}
