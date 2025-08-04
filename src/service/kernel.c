// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <io.h> // set_cr3
#include <kernel/syscall.h>
#include <mem/page.h> // allocate page
#include <service.h>
#include <std/string.h> // memcpy
#include <task/task.h>  // pid_to_task

PRIVATE syscall_status_t kern_get_pid(message_t *msg)
{
    msg3_t *m3 = &msg->m3;
    m3->l1     = msg->src;
    return SYSCALL_SUCCESS;
}

PRIVATE syscall_status_t kern_allocate_page(message_t *msg)
{
    status_t  status;
    msg2_t   *m2    = &msg->m2;
    uintptr_t vaddr = 0;
    uintptr_t paddr = 0;

    status = vmm_alloc(&running_task()->vmm, PG_SIZE, &vaddr);
    if (ERROR(status))
    {
        return SYSCALL_ERROR;
    }
    status = alloc_physical_page(1, &paddr);
    if (ERROR(status))
    {
        vmm_free(&running_task()->vmm, (uintptr_t)vaddr, PG_SIZE);
        return SYSCALL_ERROR;
    }
    page_map(running_task()->page_dir, (void *)paddr, (void *)vaddr);
    set_cr3((uint64_t)running_task()->page_dir);
    m2->p1 = (void *)vaddr;

    return SYSCALL_SUCCESS;
}

PRIVATE syscall_status_t kern_free_page(message_t *msg)
{
    msg2_t   *m2 = &msg->m2;
    uintptr_t vaddr;
    void     *paddr;

    vaddr = (uintptr_t)m2->p1;
    paddr = to_physical_address(running_task()->page_dir, (void *)vaddr);

    free_physical_page(paddr, 1);
    vmm_free(&running_task()->vmm, vaddr, PG_SIZE);
    return SYSCALL_SUCCESS;
}

PRIVATE syscall_status_t kern_read_proc_mem(message_t *msg)
{
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

PUBLIC syscall_status_t kernel_services(message_t *msg)
{
    syscall_status_t ret = SYSCALL_ERROR;
    switch (msg->type)
    {
        case KERN_GET_PID:
            ret = kern_get_pid(msg);
            break;
        /// TODO:
        case KERN_CREATE_PROCESS:
            break;
        /// TODO:
        case KERN_EXIT:
            break;
        case KERN_ALLOCATE_PAGE:
            ret = kern_allocate_page(msg);
            break;

        case KERN_FREE_PAGE:
            ret = kern_free_page(msg);
            break;

        case KERN_READ_TASK_MEM:
            ret = kern_read_proc_mem(msg);
            break;

        default:
            ret = SYSCALL_NO_SYSCALL;
            break;
    }
    return ret;
}
