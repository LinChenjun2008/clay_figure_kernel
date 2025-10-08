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

// test
#include <log.h>

// previous prototype for each function
PUBLIC syscall_status_t kern_allocate_page(message_t *msg);
PUBLIC syscall_status_t kern_free_page(message_t *msg);
PUBLIC syscall_status_t kern_read_task_page(message_t *msg);

PUBLIC syscall_status_t kern_allocate_page(message_t *msg)
{
    uint64_t   in_count = (uint64_t)msg->m[IN_KERN_ALLOCATE_PAGE_COUNT];
    uintptr_t *out_addr = (uintptr_t *)&msg->m[OUT_KERN_ALLOCATE_PAGE_ADDR];

    task_struct_t *cur_task = running_task();

    status_t  status;
    uintptr_t vaddr = 0;

    status = mm_alloc(&cur_task->mm_alloc, PG_SIZE * in_count, &vaddr);
    if (ERROR(status))
    {
        return SYSCALL_ERROR;
    }

    status = mm_add_range(&cur_task->mm_using, vaddr, PG_SIZE * in_count);
    if (ERROR(status))
    {
        mm_add_range(&cur_task->mm_alloc, vaddr, PG_SIZE * in_count);
        return SYSCALL_ERROR;
    }
    *out_addr = vaddr;
    return SYSCALL_SUCCESS;
}

PUBLIC syscall_status_t kern_free_page(message_t *msg)
{
    uintptr_t in_addr  = (uintptr_t)msg->m[IN_KERN_FREE_PAGE_ADDR];
    uint64_t  in_count = (uint64_t)msg->m[IN_KERN_FREE_PAGE_COUNT];

    task_struct_t *cur_task = running_task();

    uintptr_t vaddr;
    void     *paddr;
    vaddr = in_addr;

    mm_remove_range(&cur_task->mm_using, vaddr, PG_SIZE * in_count);
    mm_add_range(&cur_task->mm_alloc, vaddr, PG_SIZE * in_count);

    uint64_t i;
    for (i = 0; i < in_count; i++)
    {
        vaddr = in_addr + i * PG_SIZE;
        paddr = to_physical_address(cur_task->page_dir, (void *)vaddr);

        // 未映射区域,跳过
        if (paddr == NULL)
        {
            continue;
        }

        mm_remove_range(&cur_task->mm_pages, (uintptr_t)paddr, PG_SIZE);
        free_physical_page(paddr, 1);
        page_unmap(cur_task->page_dir, (void *)vaddr, 1);
    }

    page_table_activate(cur_task);
    return SYSCALL_SUCCESS;
}

PUBLIC syscall_status_t kern_read_task_page(message_t *msg)
{
    pid_t  in_pid    = (pid_t)msg->m[IN_KERN_READ_TASK_PAGE_PID];
    void  *in_addr   = (void *)msg->m[IN_KERN_READ_TASK_PAGE_ADDR];
    size_t in_count  = (size_t)msg->m[IN_KERN_READ_TASK_PAGE_COUNT];
    void  *in_buffer = (void *)msg->m[IN_KERN_READ_TASK_PAGE_BUFFER];

    task_struct_t *task  = pid_to_task(in_pid);
    void          *paddr = NULL;
    void          *addr  = NULL;

    size_t i;
    for (i = 0; i < in_count; i++)
    {
        addr  = (void *)((uintptr_t)in_buffer + PG_SIZE * i);
        paddr = (void *)((uintptr_t)in_addr + PG_SIZE * i);
        paddr = to_physical_address(task->page_dir, paddr);

        // 未映射区域,清0
        if (paddr == NULL)
        {
            memset(addr, 0, PG_SIZE);
            continue;
        }

        memcpy(addr, PHYS_TO_VIRT(paddr), PG_SIZE);
    }

    return SYSCALL_SUCCESS;
}