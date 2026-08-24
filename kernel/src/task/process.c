// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
#include <asm/page.h>
#include <asm/task.h>

#include <mem.h>
#include <print.h>
#include <ramfs.h>
#include <std/string.h>
#include <sync/atomic.h>
#include <sysinfo.h>
#include <task.h>
#include <task/struct.h>

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
    const char *file,
    uint64_t    prio,
    size_t      kstack_pages,
    size_t      ustack_pages,
    void       *arg
)
{
    ASSERT(file != NULL);
    ASSERT(kstack_pages != 0);
    ASSERT(ustack_pages != 0);

    struct task *task = allocate_task_struct();
    if (task == NULL)
    {
        return NULL;
    }

    uintptr_t kstack_base = (uintptr_t)allocate_pages(kstack_pages);
    if (kstack_base == 0)
    {
        goto fail;
    }
    init_task_struct(task, file, prio, kstack_base, kstack_pages, ustack_pages);

    task->pg_dir = create_pg_dir();
    if (task->pg_dir == NULL)
    {
        goto fail;
    }

    task->mm = allocate_mm_struct();
    if (task->mm == NULL)
    {
        goto fail;
    }
    struct vm_struct *vm = &task->mm->vm_map;
    uintptr_t         user_space_size;
    user_space_size = USER_STACK_VADDR_TOP - (ustack_pages + 1) * PG_SIZE;
    free_table_add(&vm->vm_table, USER_VADDR_START, user_space_size);

    // read executable file
    struct system_info *sys_info = get_system_info();
    void               *fp = ramfs_read(sys_info->boot_info->initramfs, file);
    if (fp == NULL)
    {
        goto fail;
    }

    create_task_context(task, fp, arg);
    atomic_inc(&get_current_task()->childs);

    cpu_task_enqueue(task);
    return task;

fail:
    destory_mm_struct(task->mm);
    free_pg_table(task->pg_dir);
    free_pages((void *)kstack_base, kstack_pages);
    destory_task_struct(task);
    return NULL;
}

void process_exit(int status)
{
    struct task *task = get_current_task();

    /// TODO: release allcated memory
    destory_mm_struct(task->mm);

    void *pg_dir = task->pg_dir;
    task->pg_dir = NULL;
    task_pg_active(task);

    free_pg_table(pg_dir);

    free_pages((void *)task->ustack_base, task->ustack_pages);
    task_exit(status);
    return;
}