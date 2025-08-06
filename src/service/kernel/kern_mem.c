// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <kernel/syscall.h>
#include <mem/page.h>   // allocate page
#include <std/string.h> // memcpy
#include <task/task.h>  // pid_to_task

// previous prototype for each function
PUBLIC syscall_status_t kern_allocate_page(message_t *msg);
PUBLIC syscall_status_t kern_free_page(message_t *msg);
PUBLIC syscall_status_t kern_read_task_mem(message_t *msg);

PUBLIC syscall_status_t kern_allocate_page(message_t *msg)
{
    // out:
    // m2.p1 = address

    msg2_t        *m2       = &msg->m2;
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
    m2->p1 = (void *)vaddr;
    return SYSCALL_SUCCESS;
}

PUBLIC syscall_status_t kern_free_page(message_t *msg)
{
    // in:
    // m2.p1 = address

    msg2_t        *m2       = &msg->m2;
    task_struct_t *cur_task = running_task();

    uintptr_t vaddr;
    void     *paddr;
    vaddr = (uintptr_t)m2->p1;
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

    msg3_t *m3    = &msg->m3;
    void   *p_src = m3->p1;
    void   *p_dst = m3->p2;
    size_t  size  = m3->l1;

    task_struct_t *task = pid_to_task(m3->i1);

    p_src = to_physical_address(task->page_dir, p_src);
    if (p_src == NULL)
    {
        return SYSCALL_ERROR;
    }
    p_src = PHYS_TO_VIRT(p_src);

    memcpy(p_dst, p_src, size);
    return SYSCALL_SUCCESS;
}