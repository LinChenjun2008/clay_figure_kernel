// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <intr.h>           // intr functions
#include <kernel/syscall.h> // sys_send_recv
#include <mem/allocator.h>  // kfree
#include <mem/page.h>       // PG_SIZE
#include <service.h>        // message type
#include <std/string.h>     // memcpy
#include <task/task.h>      // task functions,include list,spinlock,alloc_table

typedef struct
{
    uint64_t page_index;
    uint32_t page_count;
    pid_t    owner;
} page_t;

PRIVATE void mm_allocate_page(message_t *msg)
{
    void          *paddr;
    uintptr_t      vaddr_start, vaddr;
    task_struct_t *src = pid_to_task(msg->src);
    if (src->page_dir == NULL)
    {
        msg->m2.p1 = NULL;
        return;
    }
    status_t status;
    status = allocate_units(&src->vaddr_table, msg->m1.i1, &vaddr_start);
    if (ERROR(status))
    {
        msg->m2.p1 = NULL;
        return;
    }
    vaddr_start *= PG_SIZE;
    vaddr = vaddr_start;
    uint32_t i;
    for (i = 0; i < msg->m1.i1; i++)
    {
        status = alloc_physical_page(1, &paddr);
        if (ERROR(status))
        {
            while (i > 0)
            {
                vaddr -= PG_SIZE;
                page_unmap(src->page_dir, (void *)vaddr);
                i--;
            }
            free_units(&src->vaddr_table, vaddr_start, msg->m1.i1);
            msg->m2.p1 = NULL;
            return;
        }
        page_map(src->page_dir, paddr, (void *)vaddr);
        vaddr += PG_SIZE;
    }

    msg->m2.p1 = (void *)vaddr_start;
    return;
}

PRIVATE void mm_free_page(message_t *msg)
{
    task_struct_t *src   = pid_to_task(msg->src);
    uintptr_t      vaddr = (uintptr_t)msg->m3.p1;
    free_units(&src->vaddr_table, vaddr & ~(PG_SIZE - 1), msg->m3.i1);

    uint32_t i;
    for (i = 0; i < msg->m3.i1; i++)
    {
        void *paddr = to_physical_address(src->page_dir, (void *)vaddr);
        free_physical_page(paddr, 1);
        vaddr += PG_SIZE;
    }
    return;
}

PRIVATE void mm_read_prog_addr(message_t *msg)
{
    task_struct_t *src = pid_to_task(msg->src);
    task_struct_t *cur = pid_to_task(msg->m3.i1);
    if (cur == NULL)
    {
        msg->m1.i1 = 1;
        return;
    }
    uint8_t *buffer = to_physical_address(src->page_dir, msg->m3.p2);
    if (buffer == NULL)
    {
        msg->m1.i1 = 2;
        return;
    }
    buffer = PHYS_TO_VIRT(buffer);

    uint64_t *pml4t = (uint64_t *)KERNEL_PAGE_DIR_TABLE_POS;
    if (cur->page_dir != NULL)
    {
        pml4t = cur->page_dir;
    }
    uint8_t *data = to_physical_address(pml4t, msg->m3.p1);
    if (data == NULL)
    {
        msg->m1.i1 = 3;
        return;
    }
    data = PHYS_TO_VIRT(data);
    memcpy(buffer, data, msg->m3.l1);
    msg->m1.i1 = SYSCALL_SUCCESS;
    return;
}

PRIVATE void release_prog_page_pdt(uint64_t pdt)
{
    uint64_t *v_pdt;
    v_pdt = PHYS_TO_VIRT(pdt);
    int i;
    for (i = 0; i < 512; i++)
    {
        if (v_pdt[i] & 1)
        {
            free_physical_page((void *)(v_pdt[i] & ~0xfffULL), 1);
        }
    }
    kfree(v_pdt);
}

PRIVATE void release_prog_page_pdpt(uint64_t pdpt)
{
    uint64_t *v_pdpt;
    v_pdpt = PHYS_TO_VIRT(pdpt);
    int i;
    for (i = 0; i < 512; i++)
    {
        if (v_pdpt[i] & 1)
        {
            release_prog_page_pdt(v_pdpt[i] & ~0xfffULL);
        }
    }
    kfree(v_pdpt);
}

PRIVATE void release_prog_page(uint64_t *pml4t)
{
    uint64_t *v_pml4t;
    v_pml4t = PHYS_TO_VIRT(pml4t);
    int i;
    for (i = 0; i < 256; i++)
    {
        if (v_pml4t[i] & 1)
        {
            release_prog_page_pdpt(v_pml4t[i] & ~0xfffULL);
        }
    }
    kfree(v_pml4t);
}

PRIVATE void mm_exit(message_t *msg)
{
    task_struct_t *src         = pid_to_task(msg->src);
    intr_status_t  intr_status = intr_disable();
    task_man_t    *task_man    = get_task_man(src->cpu_id);
    spinlock_lock(&task_man->task_list_lock);
    if (list_find(&task_man->task_list, &src->general_tag))
    {
        list_remove(&src->general_tag);
    }
    spinlock_unlock(&task_man->task_list_lock);
    intr_set_status(intr_status);
    if (src->page_dir != NULL)
    {
        release_prog_page(src->page_dir);
        kfree(src->vaddr_table.entries);
    }
    // release kernel stack
    kfree((void *)src->kstack_base);
    task_free(src);
    return;
}

PUBLIC void mm_main()
{
    message_t msg;
    while (1)
    {
        sys_send_recv(NR_RECV, RECV_FROM_ANY, &msg);
        switch (msg.type)
        {
            case MM_ALLOCATE_PAGE:
                mm_allocate_page(&msg);
                sys_send_recv(NR_SEND, msg.src, &msg);
                break;

            case MM_FREE_PAGE:
                mm_free_page(&msg);
                break;

            case MM_READ_PROG_ADDR:
                mm_read_prog_addr(&msg);
                sys_send_recv(NR_SEND, msg.src, &msg);
                break;
            case MM_EXIT:
                mm_exit(&msg);
                break;
            default:
                break;
        }
    }
}