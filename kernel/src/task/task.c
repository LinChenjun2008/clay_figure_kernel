// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic.h>
#include <asm/page.h>
#include <asm/task.h>

#include <mem.h>
#include <print.h>
#include <std/string.h>
#include <task.h>
#include <task/struct.h>

static struct task_mgr *static_task_mgr = NULL;

static void cpu_task_init(struct cpu *cpu)
{
    memset(cpu, 0, sizeof(*cpu));
    cpu->curr_task = NULL;
    cpu->main_task = NULL;
    cpu->dead_task = NULL;
    init_spinlock(&cpu->lock);
    init_list(&cpu->task_queue);
    cpu->running_tasks = 0;

    cpu->min_vrun_time = 0;
    cpu->total_weight  = 0;
    return;
}

void set_task_mgr(struct task_mgr *task_mgr)
{
    static_task_mgr = task_mgr;
    return;
}

struct task_mgr *get_task_mgr(void)
{
    return static_task_mgr;
}

void task_init(struct boot_info *boot_info, int max_tasks)
{
    struct task_mgr *task_mgr = kmalloc(sizeof(*task_mgr), 0, 0);
    ASSERT(task_mgr != NULL);
    memset(task_mgr, 0, sizeof(*task_mgr));
    set_task_mgr(task_mgr);

    void  *task_table      = NULL;
    size_t task_table_size = sizeof(task_mgr->task_table[0]) * max_tasks;
    task_table             = kmalloc(task_table_size, 0, 0);
    ASSERT(task_table != NULL);
    memset(task_table, 0, task_table_size);

    struct cpu *cpus      = NULL;
    int         max_cpus  = apic_max_lapic_id() + 1;
    size_t      cpus_size = sizeof(task_mgr->cpus[0]) * max_cpus;
    cpus                  = kmalloc(cpus_size, 0, 0);
    ASSERT(cpus != NULL);
    memset(cpus, 0, cpus_size);

    init_spinlock(&task_mgr->lock);
    task_mgr->task_table            = task_table;
    task_mgr->max_tasks             = max_tasks;
    task_mgr->cpus                  = cpus;
    task_mgr->max_cpus              = max_cpus;
    task_mgr->kernel_page_table_pos = boot_info->page_table_pos;

    int i;
    for (i = 0; i < task_mgr->max_cpus; i++)
    {
        cpu_task_init(&task_mgr->cpus[i]);
    }
    printk("task_init: max_tasks=%d,max_cpu_id=%d.\n", max_tasks, max_cpus);
    uintptr_t kstack_base = (uintptr_t)PHYS_TO_VIRT(boot_info->stack_base);

    set_cpu_struct();
    make_main_task(kstack_base, boot_info->stack_pages);
    return;
}

void make_main_task(uintptr_t stack_base, size_t stack_pages)
{
    printk("make_main_task: stack %p, %d page(s).\n", stack_base, stack_pages);
    struct task *task = allocate_task();
    ASSERT(task != NULL);

    set_current_task(task);
    init_task_struct(task, "Main", DEFAULT_PRIO, stack_base, stack_pages, 0);

    task->cpu_id = get_current_cpu_id();
    task->status = TASK_RUNNING;

    struct cpu *cpu = get_cpu_struct(task->cpu_id);
    cpu->main_task  = task;
    return;
}

uint8_t get_current_cpu_id(void)
{
    return arch_get_current_cpu_id();
}

struct cpu *get_cpu_struct(uint8_t cpu_id)
{
    struct task_mgr *task_mgr = get_task_mgr();
    return &task_mgr->cpus[cpu_id];
}

void set_cpu_struct(void)
{
    arch_set_cpu_struct();
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

struct task *pid_to_task(pid_t pid)
{
    struct task_mgr *task_mgr = get_task_mgr();

    if (pid < 0 || pid >= task_mgr->max_tasks)
    {
        return NULL;
    }
    return task_mgr->task_table[pid];
}

static pid_t allocate_task_lock(void)
{
    struct task_mgr *task_mgr = get_task_mgr();

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

struct task *allocate_task(void)
{
    pid_t ret;

    struct task_mgr *task_mgr = get_task_mgr();

    spin_lock(&task_mgr->lock);
    ret = allocate_task_lock();
    spin_unlock(&task_mgr->lock);

    if (ret == -1)
    {
        return NULL;
    }
    struct task *task = NULL;
    task              = pid_to_task(ret);
    memset(task, 0, sizeof(*task));
    task->pid = ret;
    return task;
}

void free_task(struct task *task)
{
    if (task == NULL)
    {
        return;
    }
    struct task_mgr *task_mgr = get_task_mgr();

    pid_t pid = task->pid;

    if (pid < 0 || pid >= task_mgr->max_tasks)
    {
        return;
    }
    spin_lock(&task_mgr->lock);
    kfree(task);
    task_mgr->task_table[pid] = NULL;
    spin_unlock(&task_mgr->lock);
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

    task->ustack_base  = 0;
    task->ustack_pages = ustack_pages;
    task->ustack_sp    = NULL;

    task->cpu_id = get_current_task()->cpu_id;

    task->ppid = get_current_task()->pid;

    strncpy(task->name, name, 31);
    task->name[31] = '\0';

    task->status        = TASK_READY;
    task->preempt_count = 0;
    task->pg_dir        = NULL;

    task->prio      = prio;
    task->run_time  = 0;
    task->vrun_time = 0;

    atomic_set(&task->childs, 0);
    task->return_status = 0;
    init_list(&task->exited_childs);
    init_spinlock(&task->exited_lock);
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

    struct task *task = allocate_task();
    if (task == NULL)
    {
        goto fail;
    }

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
    free_pages((void *)kstack_base, kstack_pages);
    free_task(task);
    return NULL;
}

void task_exit(int return_value)
{
    struct task *task = get_current_task();

    task->return_status = return_value;

    /// TODO: 将子任务由Main task接管

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

static int find_child(struct list_node *node, uint64_t arg)
{
    struct task *task = NULL;
    task              = CONTAINER_OF(struct task, general_node, node);
    return task->pid == (pid_t)arg;
}

int task_release_resources(struct task *task)
{
    struct task *parent_task = pid_to_task(task->ppid);
    ASSERT(get_current_task() == parent_task);

    free_pages((void *)task->kstack_base, task->kstack_pages);
    int ret = task->return_status;
    atomic_dec(&parent_task->childs);
    free_task(task);
    return ret;
}

pid_t task_waitpid(pid_t pid, int *status, int options)
{
    // unsupport
    if (pid == 0 || pid < -1)
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
    if (pid == -1)
    {
        spin_lock(&task->exited_lock);
        node = list_pop(&task->exited_childs);
        spin_unlock(&task->exited_lock);
    }
    else
    {
        spin_lock(&task->exited_lock);
        node = list_traversal_remove(&task->exited_childs, find_child, pid);
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