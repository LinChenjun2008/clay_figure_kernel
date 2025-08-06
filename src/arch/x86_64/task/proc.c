// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/cpu.h>     // apic_id,IA32_KERNEL_GS_BASE
#include <kernel/syscall.h> // sys_send_recv
#include <mem/allocator.h>  // kmalloc,kfree
#include <mem/page.h>       // alloc_physical_page,page_map,set_page_table
#include <service.h>        // MM_EXIT
#include <std/string.h>     // memset,memcpy
#include <task/task.h>      // task struct & functions,spinlock

PRIVATE void proc_start(int (*func)(void *), uint64_t arg)
{
    int       ret_value = func((void *)arg);
    message_t msg;
    msg.type  = KERN_EXIT;
    msg.m1.i1 = ret_value;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
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
        proc_exit(-1);
        PR_LOG(LOG_FATAL, "Shuold not be here.");
        while (1) continue;
    }
    cur->ustack_base = ustack;
    cur->ustack_size = PG_SIZE;
    page_map(cur->page_dir, (void *)ustack, (void *)USER_STACK_VADDR_BASE);
    page_table_activate(cur);

    uint64_t kstack = (uint64_t)cur->context;
    kstack += sizeof(task_context_t);
    asm_switch_to_user(
        proc_start,
        func,
        kstack,
        USER_STACK_VADDR_BASE + PG_SIZE,
        EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1
    );
    PR_LOG(LOG_FATAL, "Shuold not be here.");
    return; // 应该永远不会到这里
}

PUBLIC void page_table_activate(task_struct_t *task)
{
    void *page_dir_table_pos = (void *)KERNEL_PAGE_DIR_TABLE_POS;
    if (task->page_dir != NULL)
    {
        page_dir_table_pos = task->page_dir;
    }
    set_page_table(page_dir_table_pos);
    return;
}

PUBLIC void proc_activate(task_struct_t *task)
{
    page_table_activate(task);
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
    size_t   block_size   = sizeof(*task->vmm_free.blocks);
    uint64_t total_blocks = 1024;
    void    *blocks;
    status_t status = kmalloc(block_size * total_blocks, 0, 0, &blocks);
    if (ERROR(status))
    {
        return status;
    }
    vmm_struct_init(&task->vmm_free, blocks, total_blocks);

    uintptr_t vm_start = USER_VADDR_START;
    size_t    vm_size  = (USER_STACK_VADDR_BASE - USER_VADDR_START);
    vmm_add_range(&task->vmm_free, vm_start, vm_size);

    status = kmalloc(block_size * total_blocks, 0, 0, &blocks);
    if (ERROR(status))
    {
        return status;
    }
    vmm_struct_init(&task->vmm_using, blocks, total_blocks);
    return K_SUCCESS;
}

PRIVATE status_t free_user_vaddr_table(task_struct_t *task)
{
    kfree(task->vmm_free.blocks);
    kfree(task->vmm_using.blocks);
    return K_SUCCESS;
}

PUBLIC task_struct_t *proc_execute(
    const char *name,
    uint64_t    priority,
    size_t      kstack_size,
    void       *proc
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
    create_task_struct(task, start_process, (uint64_t)proc);
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
    free_user_vaddr_table(task);
    kfree(PHYS_TO_VIRT(task->page_dir));
    kfree(kstack_base);
    task_free(task);
    return NULL;
}

PUBLIC void proc_exit(int ret_val)
{
    task_struct_t *task   = running_task();
    void          *pg_dir = task->page_dir;

    free_physical_page((void *)task->ustack_base, 1);
    // 使用内核页表,以便回收任务自身的页表
    task->page_dir = NULL;
    page_table_activate(task);

    free_page_table(pg_dir);
    free_user_vaddr_table(task);

    task_exit(ret_val);
    return;
}