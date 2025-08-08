// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/cpu.h>     // apic_id
#include <intr.h>           // intr functions
#include <io.h>             // get_cr3 get_rsp
#include <kernel/syscall.h> // sys_send_recv
#include <mem/allocator.h> // kmalloc,to_physical_address,init_alloc_physical_page
#include <mem/page.h>      // VIRT_TO_PHYS,PHYS_TO_VIRT
#include <service.h>       // MM_EXIT
#include <std/string.h>    // memset,strlen,strcpy
#include <sync/atomic.h>   // atomic functions
#include <task/task.h>     // include sse,spinlock

PRIVATE global_task_man_t *global_task_man;

PRIVATE void kernel_task(uintptr_t func, uint64_t arg)
{
    intr_enable();
    sse_init();
    ((void (*)(uint64_t))func)(arg);

    task_exit(0);
    while (1) continue;
    return;
}

PUBLIC global_task_man_t *get_global_task_man(void)
{
    return global_task_man;
}

PUBLIC task_man_t *get_task_man(uint32_t cpu_id)
{
    return &global_task_man->cpus[cpu_id];
}

PUBLIC task_struct_t *pid_to_task(pid_t pid)
{
    if (pid > MAX_PID || !task_exist(pid))
    {
        return NULL;
    }
    return &global_task_man->tasks[pid];
}

PRIVATE pid_t task_to_pid(task_struct_t *task)
{
    pid_t ret;

    ret = ((uintptr_t)task - (uintptr_t)&global_task_man->tasks);
    ret = ret / sizeof(*task);

    if (ret > MAX_PID)
    {
        return PID_NO_TASK;
    }

    return ret;
}

PUBLIC bool task_exist(pid_t pid)
{
    if (pid >= 0 && pid <= MAX_PID)
    {
        return get_global_task_man()->tasks[pid].pid != PID_NO_TASK;
    }
    return 0;
}

// running_task() can only be called in ring 0
PUBLIC task_struct_t *running_task(void)
{
    return (task_struct_t *)rdmsr(IA32_KERNEL_GS_BASE);
}

PUBLIC task_struct_t *task_alloc(void)
{
    task_struct_t *task = NULL;
    spinlock_lock(&global_task_man->tasks_lock);
    pid_t i;
    for (i = 0; i < TASKS; i++)
    {
        if (global_task_man->tasks[i].pid == PID_NO_TASK)
        {
            task = &global_task_man->tasks[i];
            memset(task, 0, sizeof(*task));
            task->pid = i;
            break;
        }
    }
    spinlock_unlock(&global_task_man->tasks_lock);
    return task;
}

PUBLIC void task_free(task_struct_t *task)
{
    if (task == NULL)
    {
        return;
    }
    spinlock_lock(&global_task_man->tasks_lock);
    task->pid = PID_NO_TASK;
    spinlock_unlock(&global_task_man->tasks_lock);
    return;
}

PRIVATE void main_task_adopt_child(pid_t ppid)
{
    task_struct_t *child_task;
    task_man_t    *child_task_man;

    task_struct_t *parent_task = pid_to_task(ppid);

    // 防止在此期间还有子任务退出
    spinlock_lock(&parent_task->child_list_lock);
    pid_t i;
    for (i = MIN_PID; i <= MAX_PID; i++)
    {
        child_task = pid_to_task(i);
        if (child_task != NULL && child_task->ppid == ppid)
        {
            child_task_man   = get_task_man(child_task->cpu_id);
            child_task->ppid = child_task_man->main_task->pid;
        }
    }
    // 已退出的子任务也转给main_task
    while (!list_empty(&parent_task->exited_child_list))
    {
        list_node_t *node = list_pop(&parent_task->exited_child_list);
        child_task        = CONTAINER_OF(task_struct_t, general_tag, node);
        child_task_man    = get_task_man(child_task->cpu_id);
        spinlock_lock(&child_task_man->main_task->child_list_lock);
        list_append(&child_task_man->main_task->exited_child_list, node);
        spinlock_unlock(&child_task_man->main_task->child_list_lock);
    }
    spinlock_unlock(&parent_task->child_list_lock);
    return;
}

PUBLIC status_t init_task_struct(
    task_struct_t *task,
    const char    *name,
    uint64_t       priority,
    uintptr_t      kstack_base,
    size_t         kstack_size
)
{
    memset(task, 0, sizeof(*task));
    task->context     = (task_context_t *)(kstack_base + kstack_size);
    task->kstack_base = kstack_base;
    task->kstack_size = kstack_size;

    task->ustack_base = 0;
    task->ustack_size = 0;

    task->pid  = task_to_pid(task);
    task->ppid = running_task()->pid;

    strncpy(task->name, name, 31);
    task->name[31] = '\0';

    task->status        = TASK_READY;
    task->preempt_count = 0;
    task->cpu_id        = running_task()->cpu_id;
    task->page_dir      = NULL;

    task->priority  = priority;
    task->run_time  = 0;
    task->vrun_time = 0; // 将由task_update设置

    task->send_to   = PID_NO_TASK;
    task->recv_from = PID_NO_TASK;
    atomic_set(&task->send_flag, 0);
    atomic_set(&task->recv_flag, 0);

    task->has_intr_msg = 0;
    init_spinlock(&task->send_lock);
    init_list(&task->sender_list);

    atomic_set(&task->childs, 0);
    init_spinlock(&task->child_list_lock);
    init_list(&task->exited_child_list);
    task->return_value = 0;

    fxsave_region_t *fxsave_region;
    status_t         status;
    status = kmalloc(sizeof(*task->fxsave_region), 0, 0, &fxsave_region);
    if (ERROR(status))
    {
        return status;
    }
    task->fxsave_region = fxsave_region;
    return K_SUCCESS;
}

PUBLIC void create_task_struct(task_struct_t *task, void *func, uint64_t arg)
{
    ASSERT(task->context != NULL);
    uintptr_t kstack = (uintptr_t)task->context;
    kstack -= sizeof(uintptr_t);
    *(uintptr_t *)kstack = (uintptr_t)kernel_task;
    kstack -= sizeof(task_context_t);
    task->context           = (task_context_t *)kstack;
    task_context_t *context = task->context;
    context->rsi            = (uint64_t)arg;
    context->rdi            = (uint64_t)func;
    return;
}

PUBLIC task_struct_t *task_start(
    const char *name,
    uint64_t    priority,
    size_t      kstack_size,
    void       *func,
    uint64_t    arg
)
{
    if (kstack_size & (kstack_size - 1))
    {
        return NULL;
    }
    task_struct_t *task = task_alloc();
    if (task == NULL)
    {
        return NULL;
    }
    uintptr_t kstack_base;
    status_t  status = kmalloc(kstack_size, 0, 0, &kstack_base);
    if (ERROR(status))
    {
        task_free(task);
        return NULL;
    }

    init_task_struct(task, name, priority, kstack_base, kstack_size);
    create_task_struct(task, func, arg);

    task_struct_t *parent_task = pid_to_task(task->ppid);
    atomic_inc(&parent_task->childs);

    task_man_t *task_man = get_task_man(task->cpu_id);
    spinlock_lock(&task_man->task_list_lock);
    task_list_insert(task_man, task);
    spinlock_unlock(&task_man->task_list_lock);
    return task;
}

PUBLIC void task_exit(int ret_val)
{
    task_struct_t *task = running_task();
    task->return_value  = ret_val;

    /// TODO: 处理未完成的IPC

    main_task_adopt_child(task->pid);
    // 通知父进程任务退出
    task_block(TASK_DIED);
    return;
}

PUBLIC int task_has_exited_child(pid_t pid)
{
    task_struct_t *task = pid_to_task(pid);
    spinlock_lock(&task->child_list_lock);
    int ret = !list_empty(&task->exited_child_list);
    spinlock_unlock(&task->child_list_lock);
    return ret;
}

PUBLIC int task_release_resource(pid_t pid)
{
    task_struct_t *task        = pid_to_task(pid);
    task_struct_t *parent_task = pid_to_task(task->ppid);
    ASSERT(parent_task->pid == running_task()->pid);
    kfree(task->fxsave_region);
    kfree((void *)task->kstack_base);

    // 获取返回值
    int return_value = task->return_value;

    atomic_dec(&parent_task->childs);

    task_free(task);
    return return_value;
}

PRIVATE void idle_task()
{
    while (1)
    {
        task_block(TASK_BLOCKED);
    }
    return;
}

PRIVATE void make_main_task(void)
{
    task_struct_t *main_task = task_alloc();
    main_task->cpu_id        = apic_id();

    task_man_t *task_man = get_task_man(main_task->cpu_id);
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)main_task);

    init_task_struct(
        main_task,
        "Main task",
        DEFAULT_PRIORITY,
        (uintptr_t)PHYS_TO_VIRT(KERNEL_STACK_BASE),
        KERNEL_STACK_SIZE
    );
    main_task->status   = TASK_RUNNING; // main_task已经在运行
    task_man->main_task = main_task;
    return;
}

PUBLIC void create_idle_task(void)
{
    task_struct_t *task     = running_task();
    task_man_t    *task_man = get_task_man(task->cpu_id);

    task_struct_t *idle = task_start("idle", IDLE_PRIORITY, 4096, idle_task, 0);
    task_man->idle_task = idle;

    return;
}

PUBLIC void task_init(void)
{
    uintptr_t addr;
    uint64_t  pages  = sizeof(*global_task_man) / PG_SIZE + 1;
    status_t  status = alloc_physical_page_sub(pages, &addr);

    PANIC(ERROR(status), "Can not allocate memory for task manager.");

    global_task_man = PHYS_TO_VIRT(addr);
    memset(global_task_man, 0, sizeof(*global_task_man));
    int i;
    for (i = 0; i < TASKS; i++)
    {
        global_task_man->tasks[i].pid = PID_NO_TASK;
    }
    for (i = 0; i < NR_CPUS; i++)
    {
        task_man_t *task_man = &global_task_man->cpus[i];

        init_list(&task_man->task_list);
        init_spinlock(&task_man->task_list_lock);

        init_list(&task_man->waiting_list);

        task_man->min_vrun_time = 0;
        task_man->running_tasks = 0;
        task_man->total_weight  = 0;
        task_man->main_task     = NULL;
        task_man->idle_task     = NULL;
    }
    init_spinlock(&global_task_man->tasks_lock);

    make_main_task();
    create_idle_task();
    return;
}