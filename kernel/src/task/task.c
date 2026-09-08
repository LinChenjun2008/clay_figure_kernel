// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic.h>
#include <asm/task.h>

#include <mem/allocator.h>
#include <mem/page.h>
#include <panic.h>
#include <print.h>
#include <std/stdio.h>
#include <std/string.h>
#include <syscall/ipc.h>
#include <sysinfo.h>
#include <task.h>
#include <task/process.h>
#include <task/schedule.h>
#include <task/struct.h>

static void cpu_task_init(struct cpu *cpu, struct task_mgr *task_mgr)
{
    memset(cpu, 0, sizeof(*cpu));
    cpu->curr_task = NULL;
    cpu->main_task = NULL;
    cpu->dead_task = NULL;

    cpu->task_mgr = task_mgr;

    init_spinlock(&cpu->lock);
    cpu->running_tasks = 0;
    init_list(&cpu->task_queue);

    cpu->min_vrun_time = 0;
    cpu->total_weight  = 0;
    return;
}

void task_init(struct system_info *system_info, int max_tasks)
{
    struct task_mgr *task_mgr = system_info->task_mgr;

    void  *task_table      = NULL;
    size_t task_table_size = sizeof(task_mgr->task_table[0]) * max_tasks;
    task_table             = kmalloc(task_table_size, 0, 0);
    ASSERT(task_table != NULL);
    memset(task_table, 0, task_table_size);

    void  *pid_table      = NULL;
    size_t pid_table_size = sizeof(pid_t) * max_tasks;
    pid_table             = kmalloc(pid_table_size, 0, 0);
    ASSERT(pid_table != NULL);
    memset(pid_table, 0, pid_table_size);

    struct cpu *cpus      = NULL;
    int         max_cpus  = apic_max_lapic_id() + 1;
    size_t      cpus_size = sizeof(task_mgr->cpus[0]) * max_cpus;
    cpus                  = kmalloc(cpus_size, 0, 0);
    ASSERT(cpus != NULL);
    memset(cpus, 0, cpus_size);

    struct boot_info *boot_info = system_info->boot_info;

    init_spinlock(&task_mgr->lock);
    task_mgr->task_table            = task_table;
    task_mgr->pid_table             = pid_table;
    task_mgr->max_tasks             = max_tasks;
    task_mgr->cpus                  = cpus;
    task_mgr->max_cpus              = max_cpus;
    task_mgr->kernel_page_table_pos = boot_info->page_table_pos;

    int i;
    for (i = 0; i < task_mgr->max_cpus; i++)
    {
        cpu_task_init(&task_mgr->cpus[i], task_mgr);
    }
    printk("task_init: max_tasks=%d,max_cpu_id=%d.\n", max_tasks, max_cpus);
    uintptr_t kstack_base = (uintptr_t)PHYS_TO_VIRT(boot_info->stack_base);

    struct cpu *cpu = get_curr_cpu_struct();
    set_cpu_struct(cpu);

    make_main_task(kstack_base, boot_info->stack_pages);

    struct task *init = process_execute("init", IDLE_PRIO, 1, 1, NULL);
    ASSERT(init->pid == 1);

    return;
}

void make_main_task(uintptr_t stack_base, size_t stack_pages)
{
    printk("make_main_task: stack %p, %d page(s).\n", stack_base, stack_pages);
    struct task *task = allocate_task_struct();
    ASSERT(task != NULL);

    set_current_task(task);
    char name[32];
    sprintf(name, "main[%d]", get_current_cpu_id());
    init_task_struct(task, name, DEFAULT_PRIO, stack_base, stack_pages, 0);

    task->cpu_id = get_current_cpu_id();
    task->status = TASK_RUNNING;

    struct cpu *cpu = get_cpu_struct(task->cpu_id);
    cpu->main_task  = task;
    return;
}

void set_current_task(struct task *task)
{
    uint8_t     cpu_id = get_current_cpu_id();
    struct cpu *cpu    = get_cpu_struct(cpu_id);
    cpu->curr_task     = task;
    return;
}

struct task *get_current_task(void)
{
    uint8_t     cpu_id = get_current_cpu_id();
    struct cpu *cpu    = get_cpu_struct(cpu_id);
    return cpu->curr_task;
}

int check_pid_avaiability(pid_t pid)
{
    struct task_mgr *task_mgr = get_task_mgr();

    pid_t   task_index = GET_FIELD(pid, PID_INDEX);
    uint8_t count      = GET_FIELD(pid, PID_COUNT);

    if (task_index < 0 || task_index >= task_mgr->max_tasks)
    {
        return 0;
    }
    if (task_mgr->pid_table[task_index] != count)
    {
        return 0;
    }
    return 1;
}

struct task *pid_to_task(pid_t pid)
{
    if (!check_pid_avaiability(pid))
    {
        return NULL;
    }
    pid_t task_index = GET_FIELD(pid, PID_INDEX);

    struct task_mgr *task_mgr = get_task_mgr();
    return task_mgr->task_table[task_index];
}

static pid_t allocate_pid(struct task_mgr *task_mgr, pid_t task_index)
{
    pid_t count = task_mgr->pid_table[task_index];
    pid_t ret   = 0;
    ret += SET_FIELD(0, PID_INDEX, task_index);
    ret += SET_FIELD(0, PID_COUNT, count);
    return ret;
}

static pid_t allocate_task_lock(struct task_mgr *task_mgr)
{
    pid_t i;
    for (i = 0; i < task_mgr->max_tasks; i++)
    {
        if (task_mgr->task_table[i] == NULL)
        {
            break;
        }
    }
    if (i == task_mgr->max_tasks)
    {
        return -1;
    };
    task_mgr->task_table[i] = kmalloc(sizeof(*task_mgr->task_table[0]), 0, 0);
    if (task_mgr->task_table[i] == NULL)
    {
        return -1;
    }
    return i;
}

struct task *allocate_task_struct(void)
{
    struct task_mgr *task_mgr = get_task_mgr();

    spin_lock(&task_mgr->lock);
    pid_t index = allocate_task_lock(task_mgr);
    pid_t pid   = allocate_pid(task_mgr, index);
    spin_unlock(&task_mgr->lock);

    if (index == -1)
    {
        return NULL;
    }
    struct task *task = NULL;
    task              = pid_to_task(pid);
    memset(task, 0, sizeof(*task));
    task->pid = pid;
    return task;
}

void destory_task_struct(struct task *task)
{
    if (task == NULL)
    {
        return;
    }
    struct task_mgr *task_mgr = get_task_mgr();

    pid_t pid        = task->pid;
    pid_t task_index = GET_FIELD(pid, PID_INDEX);

    if (!check_pid_avaiability(pid))
    {
        return;
    }
    spin_lock(&task_mgr->lock);
    kfree(task);
    task_mgr->pid_table[task_index]++;
    task_mgr->task_table[task_index] = NULL;
    spin_unlock(&task_mgr->lock);
    return;
}

void init_task_struct(
    struct task *task,
    const char  *name,
    uint64_t     prio,
    uintptr_t    kstack_base,
    size_t       kstack_pages,
    size_t       ustack_pages
)
{
    task->context      = (void *)(kstack_base + kstack_pages * PG_SIZE);
    task->kstack_base  = kstack_base;
    task->kstack_pages = kstack_pages;

    task->ustack_pages = ustack_pages;
    task->ustack_sp    = 0;

    task->cpu_id = get_current_task()->cpu_id;

    task->ppid = get_current_task()->pid;

    strncpy(task->name, name, 31);
    task->name[31] = '\0';

    init_spinlock(&task->lock);
    task->status        = TASK_READY;
    task->block_count   = 0;
    task->preempt_count = 0;
    task->pg_dir        = 0;

    task->prio      = prio;
    task->run_time  = 0;
    task->vrun_time = 0;

    task->mm = NULL;

    atomic_set(&task->childs, 0);
    task->return_status = 0;
    init_list(&task->exited_childs);
    init_spinlock(&task->exited_lock);

    init_mailbox(&task->mailbox);

    return;
}

struct task *task_start(
    const char *name,
    uint64_t    prio,
    size_t      kstack_pages,
    void       *func,
    void       *arg
)
{
    ASSERT(name != NULL);
    ASSERT(kstack_pages != 0);
    ASSERT(func != 0);

    struct task *task = allocate_task_struct();
    if (task == NULL)
    {
        return NULL;
    }
    ASSERT(task->kstack_base == 0 && task->kstack_pages == 0);

    uintptr_t kstack_base = (uintptr_t)allocate_pages(kstack_pages);
    if (kstack_base == 0)
    {
        goto fail;
    }
    init_task_struct(task, name, prio, kstack_base, kstack_pages, 0);
    create_task_context(task, func, arg);

    atomic_inc(&get_current_task()->childs);

    cpu_task_enqueue(task);
    return task;

fail:
    free_pages((void *)task->kstack_base, task->kstack_pages);
    destory_task_struct(task);
    return NULL;
}
