// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <kernel/syscall.h>
#include <mem/page.h> // allocate page
#include <service.h>
#include <std/string.h> // memcpy
#include <task/task.h>  // pid_to_task

// previous prototype for each function
PUBLIC syscall_status_t kern_allocate_page(message_t *msg);
PUBLIC syscall_status_t kern_free_page(message_t *msg);
PUBLIC syscall_status_t kern_read_task_mem(message_t *msg);

PUBLIC syscall_status_t kern_allocate_page(message_t *msg)
{
    uintptr_t *out_addr = (uintptr_t *)&msg->m[OUT_KERN_ALLOCATE_PAGE_ADDR];

    task_struct_t *cur_task = running_task();

    status_t  status;
    uintptr_t vaddr = 0;

    status = vmm_alloc(&cur_task->vmm_free, PG_SIZE, &vaddr);
    if (ERROR(status))
    {
        return SYSCALL_ERROR;
    }

    status = vmm_add_range(&cur_task->vmm_using, vaddr, PG_SIZE);
    if (ERROR(status))
    {
        vmm_add_range(&cur_task->vmm_free, vaddr, PG_SIZE);
        return SYSCALL_ERROR;
    }
    *out_addr = vaddr;
    return SYSCALL_SUCCESS;
}

PUBLIC syscall_status_t kern_free_page(message_t *msg)
{
    uintptr_t in_addr = (uintptr_t)msg->m[IN_KERN_FREE_PAGE_ADDR];

    task_struct_t *cur_task = running_task();

    uintptr_t vaddr;
    void     *paddr;
    vaddr = in_addr;
    paddr = to_physical_address(cur_task->page_dir, (void *)vaddr);

    vmm_remove_range(&cur_task->vmm_using, vaddr, PG_SIZE);
    vmm_add_range(&cur_task->vmm_free, vaddr, PG_SIZE);

    free_physical_page(paddr, 1);
    page_unmap(cur_task->page_dir, (void *)vaddr);

    page_table_activate(cur_task);
    return SYSCALL_SUCCESS;
}

PUBLIC syscall_status_t kern_read_task_mem(message_t *msg)
{
    // in:
    // m3.p1 = src
    // m3.p2 = dst
    // m3.l1 = read size
    // m3.i1 = target task pid

    pid_t  in_pid    = (pid_t)msg->m[IN_KERN_READ_TASK_MEM_PID];
    void  *in_addr   = (void *)msg->m[IN_KERN_READ_TASK_MEM_ADDR];
    size_t in_size   = (size_t)msg->m[IN_KERN_READ_TASK_MEM_SIZE];
    void  *in_buffer = (void *)msg->m[IN_KERN_READ_TASK_MEM_BUFFER];

    task_struct_t *task = pid_to_task(in_pid);

    in_addr = to_physical_address(task->page_dir, in_addr);
    if (in_addr == NULL)
    {
        return SYSCALL_ERROR;
    }
    in_addr = PHYS_TO_VIRT(in_addr);

    memcpy(in_buffer, in_addr, in_size);
    return SYSCALL_SUCCESS;
}