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
#include <task/signal.h>
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

static void init_task_table(struct task_table *task_table)
{
    init_spinlock(&task_table->lock);

    size_t pid_map_size = (MAX_PID >> 3) * sizeof(uint8_t);
    void  *map          = kmalloc(pid_map_size, 0, 0);
    ASSERT(map != NULL);
    init_bitmap(&task_table->pid_map, pid_map_size, map);

    task_table->task_root.count = 0;
    int i;
    for (i = 0; i < SLOTS_PER_LEVEL; i++)
    {
        task_table->task_root.slots[i] = NULL;
    }
    task_table->last_pid = 0;
}

void task_init(struct system_info *system_info)
{
    struct task_mgr *task_mgr = system_info->task_mgr;

    struct cpu *cpus      = NULL;
    int         max_cpus  = apic_max_lapic_id() + 1;
    size_t      cpus_size = sizeof(task_mgr->cpus[0]) * max_cpus;
    cpus                  = kmalloc(cpus_size, 0, 0);
    ASSERT(cpus != NULL);
    memset(cpus, 0, cpus_size);

    struct boot_info *boot_info = system_info->boot_info;

    init_task_table(&task_mgr->task_table);
    task_mgr->cpus                  = cpus;
    task_mgr->max_cpus              = max_cpus;
    task_mgr->kernel_page_table_pos = boot_info->page_table_pos;

    int i;
    for (i = 0; i < task_mgr->max_cpus; i++)
    {
        cpu_task_init(&task_mgr->cpus[i], task_mgr);
    }
    printk("task_init: max_cpu_id=%d.\n", max_cpus);
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
    init_task_struct(task, name, IDLE_PRIO, stack_base, stack_pages, 0);

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

struct task *pid_to_task(pid_t pid)
{
    if (pid <= 0)
    {
        return NULL;
    }
    struct task *ret = NULL;

    int slot_1 = GET_FIELD(pid, TASK_SLOT_L1);
    int slot_2 = GET_FIELD(pid, TASK_SLOT_L2);
    int slot_3 = GET_FIELD(pid, TASK_SLOT_L3);

    struct task_mgr   *task_mgr   = get_task_mgr();
    struct task_table *task_table = &task_mgr->task_table;

    spin_lock(&task_table->lock);

    struct task_slots *slot = &task_table->task_root;
    if (slot->slots[slot_1] == NULL)
    {
        goto end;
    }
    slot = slot->slots[slot_1];

    if (slot->slots[slot_2] == NULL)
    {
        goto end;
    }
    slot = slot->slots[slot_2];

    if (slot->slots[slot_3] == NULL)
    {
        goto end;
    }
    ret = slot->slots[slot_3];
end:
    spin_unlock(&task_table->lock);

    return ret;
}

int check_pid_avaiability(pid_t pid)
{
    return pid_to_task(pid) != NULL;
}

pid_t allocate_pid(void)
{
    struct task_mgr   *task_mgr   = get_task_mgr();
    struct task_table *task_table = &task_mgr->task_table;
    spin_lock(&task_table->lock);
    int   ret = -1;
    pid_t i;
    for (i = task_table->last_pid + 1; i < MAX_PID; i++)
    {
        if (bitmap_scan_test(&task_table->pid_map, i))
        {
            continue;
        }
        ret = i;
        break;
    }
    // 如果没找到,则从头查找
    if (i == MAX_PID)
    {
        for (i = RESERVED_PIDS; i < task_table->last_pid; i++)
        {
            if (bitmap_scan_test(&task_table->pid_map, i))
            {
                continue;
            }
            ret = i;
            break;
        }
    }
    if (ret == -1)
    {
        goto end;
    }
    bitmap_set(&task_table->pid_map, ret, 1, 1);
    task_table->last_pid = ret;
end:
    spin_unlock(&task_table->lock);
    return ret;
}

void release_pid(pid_t pid)
{
    struct task_mgr   *task_mgr   = get_task_mgr();
    struct task_table *task_table = &task_mgr->task_table;
    spin_lock(&task_table->lock);
    ASSERT(bitmap_scan_test(&task_table->pid_map, pid));
    bitmap_set(&task_table->pid_map, pid, 0, 1);
    spin_unlock(&task_table->lock);
    return;
}

int pid_table_insert(struct task *task)
{
    int   ret = -1;
    pid_t pid = task->pid;

    struct task_mgr   *task_mgr   = get_task_mgr();
    struct task_table *task_table = &task_mgr->task_table;

    spin_lock(&task_table->lock);
    int slot_1 = GET_FIELD(pid, TASK_SLOT_L1);
    int slot_2 = GET_FIELD(pid, TASK_SLOT_L2);
    int slot_3 = GET_FIELD(pid, TASK_SLOT_L3);

    struct task_slots *slot = &task_table->task_root;
    if (slot->slots[slot_1] == NULL)
    {
        struct task_slots *new_slot = kmalloc(sizeof(*new_slot), 0, 0);
        if (new_slot == NULL)
        {
            goto end;
        }
        memset(new_slot, 0, sizeof(*new_slot));
        new_slot->count = 0;
        slot->count++;
        slot->slots[slot_1] = new_slot;
    }
    slot = slot->slots[slot_1];

    if (slot->slots[slot_2] == NULL)
    {
        struct task_slots *new_slot = kmalloc(sizeof(*new_slot), 0, 0);
        if (new_slot == NULL)
        {
            goto end;
        }
        memset(new_slot, 0, sizeof(*new_slot));
        new_slot->count = 0;
        slot->count++;
        slot->slots[slot_2] = new_slot;
    }
    slot = slot->slots[slot_2];

    ASSERT(slot->slots[slot_3] == NULL);
    ASSERT(slot->count < SLOTS_PER_LEVEL);

    slot->slots[slot_3] = task;
    slot->count++;
    ret = 0;
end:
    spin_unlock(&task_table->lock);
    return ret;
}

void pid_table_remove(struct task *task)
{
    pid_t pid    = task->pid;
    int   slot_1 = GET_FIELD(pid, TASK_SLOT_L1);
    int   slot_2 = GET_FIELD(pid, TASK_SLOT_L2);
    int   slot_3 = GET_FIELD(pid, TASK_SLOT_L3);

    struct task_mgr   *task_mgr   = get_task_mgr();
    struct task_table *task_table = &task_mgr->task_table;

    spin_lock(&task_table->lock);

    struct task_slots *slots_1, *slots_2, *slots_3;
    slots_1 = &task_table->task_root;
    slots_2 = slots_1->slots[slot_1];
    slots_3 = slots_2->slots[slot_2];
    ASSERT(slots_3->slots[slot_3] == task);
    slots_3->slots[slot_3] = NULL;
    slots_3->count--;
    if (slots_3->count == 0)
    {
        kfree(slots_3);
        slots_2->slots[slot_2] = NULL;
        slots_2->count--;
    }
    if (slots_2->count == 0)
    {
        kfree(slots_2);
        slots_1->slots[slot_1] = NULL;
        slots_1->count--;
    }

    spin_unlock(&task_table->lock);
    return;
}

struct task *allocate_task_struct(void)
{
    struct task *task = kmalloc(sizeof(*task), 0, 0);
    memset(task, 0, sizeof(*task));

    return task;
}

void destory_task_struct(struct task *task)
{
    if (task == NULL)
    {
        return;
    }
    kfree(task);

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
    memset(task, 0, sizeof(*task));
    task->context      = (void *)(kstack_base + kstack_pages * PG_SIZE);
    task->kstack_base  = kstack_base;
    task->kstack_pages = kstack_pages;

    task->ustack_pages = ustack_pages;
    task->ustack_sp    = 0;

    task->cpu_id = 0;

    task->pid  = 0;
    task->ppid = 0;

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

    task->return_status = 0;

    init_spinlock(&task->childs_lock);
    init_list(&task->childs_list);
    init_list(&task->exited_childs);

    init_mailbox(&task->mailbox);
    init_signal(&task->signal);

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
    task->kstack_base  = 0;
    task->kstack_pages = 0;

    uintptr_t kstack_base = (uintptr_t)allocate_pages(kstack_pages);
    if (kstack_base == 0)
    {
        goto fail;
    }
    init_task_struct(task, name, prio, kstack_base, kstack_pages, 0);
    create_task_context(task, func, arg);

    task->pid = allocate_pid();
    if (task->pid == -1)
    {
        goto fail;
    }
    task->ppid = get_current_task()->pid;
    if (pid_table_insert(task) < 0)
    {
        release_pid(task->pid);
        goto fail;
    }

    struct task *curr = get_current_task();
    spin_lock(&curr->childs_lock);
    list_append(&curr->childs_list, &task->parent_node);
    spin_unlock(&curr->childs_lock);

    cpu_task_enqueue(task);
    return task;

fail:
    free_pages((void *)task->kstack_base, task->kstack_pages);
    destory_task_struct(task);
    return NULL;
}
