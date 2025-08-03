// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/cpu.h>     // apic_id,IA32_KERNEL_GS_BASE
#include <io.h>             // set_cr3
#include <kernel/syscall.h> // sys_send_recv
#include <mem/allocator.h>  // kmalloc,kfree
#include <mem/page.h>       // alloc_physical_page,page_map
#include <service.h>        // MM_EXIT
#include <std/string.h>     // memset,memcpy
#include <task/task.h>      // task struct & functions,spinlock

PRIVATE void prog_exit(int (*func)(void *), uint64_t arg)
{
    // int ret_value = func((void *)arg);
    func((void *)arg);
    /// TODO: Exit process
    while (1) continue;
}

/**
 * @brief 切换到用户态
 * @param func 在用户态中执行的函数(rdi)
 * @param arg 在用户态函数的参数 (rsi)
 * @param kstack 内核栈地址 (rdx)
 * @param ustack 用户栈地址 (rcx)
 * @param rflags 用户态rflags寄存器值(r8)
 */
extern void ASMLINKAGE asm_switch_to_user(
    void    *func,
    void    *arg,
    uint64_t kstack,
    uint64_t ustack,
    uint64_t rflags
);
PRIVATE void start_process(void *process)
{
    void          *func = process;
    task_struct_t *cur  = running_task();

    uintptr_t ustack;
    status_t  status = alloc_physical_page(1, &ustack);
    if (ERROR(status))
    {
        PR_LOG(LOG_ERROR, "Alloc User Stack error.\n");
        /// TODO: Exit process
        PR_LOG(LOG_FATAL, "Shuold not be here.");
        while (1) continue;
    }
    cur->ustack_base = ustack;
    cur->ustack_size = PG_SIZE;
    page_map(cur->page_dir, (void *)ustack, (void *)USER_STACK_VADDR_BASE);
    set_cr3((uint64_t)cur->page_dir);

    uint64_t kstack = (uint64_t)cur->context;
    kstack += sizeof(task_context_t);
    asm_switch_to_user(
        prog_exit,
        func,
        kstack,
        USER_STACK_VADDR_BASE + PG_SIZE,
        EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1
    );
    PR_LOG(LOG_FATAL, "Shuold not be here.");
    return; // 应该永远不会到这里
}

PRIVATE void page_dir_activate(task_struct_t *task)
{
    uint64_t page_dir_table_pos = KERNEL_PAGE_DIR_TABLE_POS;
    if (task->page_dir != NULL)
    {
        page_dir_table_pos = (uint64_t)task->page_dir;
    }
    set_cr3(page_dir_table_pos);
    return;
}

PUBLIC void prog_activate(task_struct_t *task)
{
    page_dir_activate(task);
    if (task->page_dir != NULL)
    {
        update_tss_rsp0(task);
    }
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)task);
    return;
}

PRIVATE uint64_t *create_page_dir(void)
{
    uint64_t *pgdir_v;
    status_t  status = kmalloc(PT_SIZE, 0, PT_SIZE, &pgdir_v);
    if (ERROR(status))
    {
        return NULL;
    }
    memset(pgdir_v, 0, PT_SIZE);
    memcpy(
        pgdir_v + 0x100,
        (uint64_t *)PHYS_TO_VIRT(KERNEL_PAGE_DIR_TABLE_POS) + 0x100,
        PT_SIZE / 2
    );
    return (uint64_t *)VIRT_TO_PHYS(pgdir_v);
}

PRIVATE status_t user_vaddr_table_init(task_struct_t *task)
{
    size_t   entry_size        = sizeof(*task->vaddr_table.entries);
    uint64_t number_of_entries = 1024;
    void    *entries;
    status_t status = kmalloc(entry_size * number_of_entries, 0, 0, &entries);
    if (ERROR(status))
    {
        return status;
    }
    allocate_table_init(&task->vaddr_table, entries, number_of_entries);

    uint64_t index = USER_VADDR_START / PG_SIZE;
    uint64_t cnt   = (USER_STACK_VADDR_BASE - USER_VADDR_START) / PG_SIZE;
    free_units(&task->vaddr_table, index, cnt);
    return K_SUCCESS;
}

PUBLIC task_struct_t *prog_execute(
    const char *name,
    uint64_t    priority,
    size_t      kstack_size,
    void       *prog
)
{
    ASSERT(!(kstack_size & (kstack_size - 1)));
    status_t status;

    task_struct_t *task = task_alloc();
    if (task == NULL)
    {
        goto fail;
    }

    void *kstack_base = NULL;

    status = kmalloc(kstack_size, 0, 0, &kstack_base);
    ASSERT(!ERROR(status));
    if (ERROR(status))
    {
        goto fail;
    }

    init_task_struct(task, name, priority, (uintptr_t)kstack_base, kstack_size);
    create_task_struct(task, start_process, (uint64_t)prog);
    task->page_dir = create_page_dir();
    ASSERT(task->page_dir != NULL);
    if (task->page_dir == NULL)
    {
        PR_LOG(LOG_ERROR, "Can not alloc memory for task page table.\n");
        goto fail;
    }
    status = user_vaddr_table_init(task);
    ASSERT(!ERROR(status));
    if (ERROR(status))
    {
        PR_LOG(LOG_ERROR, "Can not init vaddr table.\n");
        goto fail;
    }
    task_man_t *task_man = get_task_man(task->cpu_id);
    spinlock_lock(&task_man->task_list_lock);
    task_list_insert(task_man, task);
    spinlock_unlock(&task_man->task_list_lock);
    return task;

fail:
    kfree(kstack_base);
    kfree(PHYS_TO_VIRT(task->page_dir));
    task_free(task);
    return NULL;
}