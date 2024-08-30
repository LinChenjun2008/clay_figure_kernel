/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.
Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/global.h>
#include <kernel/syscall.h>  // sys_send_recv
#include <task/task.h>       // task functions,include list,spinlock,alloc_table
#include <service.h>         // message type
#include <mem/mem.h>         // memory functions
#include <intr.h>            // intr functions
#include <std/string.h>      // memcpy

#include <log.h>

extern taskmgr_t *tm;

PRIVATE void mm_allocate_page(message_t *msg)
{
    void *paddr;
    addr_t vaddr_start,vaddr;
    task_struct_t *src = pid2task(msg->src);
    if (src->page_dir == NULL)
    {
        msg->m2.p1 = NULL;
        return;
    }

    status_t status = allocate_units(&src->vaddr_table,msg->m1.i1,&vaddr_start);
    if (ERROR(status))
    {
        msg->m2.p1 = NULL;
        return;
    }
    vaddr_start *= PG_SIZE;
    vaddr = vaddr_start;
    uint32_t i;
    for (i = 0;i < msg->m1.i1;i++)
    {
        status_t status = alloc_physical_page(1,&paddr);
        if (ERROR(status))
        {
            while (i > 0)
            {
                vaddr -= PG_SIZE;
                page_unmap(src->page_dir,(void*)vaddr);
                i--;
            }
            free_units(&src->vaddr_table,vaddr_start,msg->m1.i1);
            msg->m2.p1 = NULL;
            return;
        }
        page_map(src->page_dir,paddr,(void*)vaddr);
        vaddr += PG_SIZE;
    }

    msg->m2.p1 = (void*)vaddr_start;
    return;
}

PRIVATE void mm_free_page(message_t *msg)
{
    task_struct_t *src = pid2task(msg->src);
    addr_t vaddr = (addr_t)msg->m3.p1;
    free_units(&src->vaddr_table,(addr_t)vaddr & ~(PG_SIZE - 1),msg->m3.i1);

    uint32_t i;
    for (i = 0;i < msg->m3.i1;i++)
    {
        void *paddr = to_physical_address(src->page_dir,(void*)vaddr);
        free_physical_page(paddr,1);
        vaddr += PG_SIZE;
    }
    return;
}

PRIVATE void mm_read_prog_addr(message_t *msg)
{
    task_struct_t *src = pid2task(msg->src);
    task_struct_t *cur = pid2task(msg->m3.i1);
    if (cur == NULL)
    {
        msg->m1.i1 = 1;
        return;
    }
    uint8_t *buffer = to_physical_address(src->page_dir,msg->m3.p2);
    if (buffer == NULL)
    {
        msg->m1.i1 = 2;
        return;
    }
    buffer = KADDR_P2V(buffer);

    uint64_t *pml4t = (uint64_t*)KERNEL_PAGE_DIR_TABLE_POS;
    if (cur->page_dir != NULL)
    {
        pml4t = cur->page_dir;
    }
    uint8_t *data = to_physical_address(pml4t,msg->m3.p1);
    if (data == NULL)
    {
        msg->m1.i1 = 3;
        return;
    }
    data = KADDR_P2V(data);
    memcpy(buffer,data,msg->m3.l1);
    msg->m1.i1 = SYSCALL_SUCCESS;
    return;
}

PRIVATE void release_prog_page_pdt(uint64_t pdt)
{
    uint64_t *v_pdt;
    v_pdt = KADDR_P2V(pdt);
    int i;
    for (i = 0;i < 512;i++)
    {
        if (v_pdt[i] & 1)
        {
            free_physical_page((void*)(v_pdt[i] & ~0xfffULL),1);
        }
    }
    pfree((void*)pdt);
}

PRIVATE void release_prog_page_pdpt(uint64_t pdpt)
{
    uint64_t *v_pdpt;
    v_pdpt = KADDR_P2V(pdpt);
    int i;
    for (i = 0;i < 512;i++)
    {
        if (v_pdpt[i] & 1)
        {
            release_prog_page_pdt(v_pdpt[i] & ~0xfffULL);
        }
    }
    pfree((void*)pdpt);
}

PRIVATE void release_prog_page(uint64_t *pml4t)
{
    uint64_t *v_pml4t;
    v_pml4t = KADDR_P2V(pml4t);
    int i;
    for (i = 0;i < 256;i++)
    {
        if (v_pml4t[i] & 1)
        {
            release_prog_page_pdpt(v_pml4t[i] & ~0xfffULL);
        }
    }
    pfree(pml4t);
}

PRIVATE void mm_exit(message_t *msg)
{
    task_struct_t *src = pid2task(msg->src);
    intr_status_t intr_status = intr_disable();
    spinlock_lock(&tm->task_list_lock[src->cpu_id]);
    if (list_find(&tm->task_list[src->cpu_id],&src->general_tag))
    {
        list_remove(&src->general_tag);
    }
    spinlock_unlock(&tm->task_list_lock[src->cpu_id]);
    intr_set_status(intr_status);
    if (src->page_dir != NULL)
    {
        release_prog_page(src->page_dir);
        pfree(KADDR_V2P(src->vaddr_table.entries));
    }
    // release kernel stack
    pfree(KADDR_V2P(src->kstack_base));
    task_free(msg->src);
    return;
}

PUBLIC void mm_main()
{
    message_t msg;
    while(1)
    {
        sys_send_recv(NR_RECV,RECV_FROM_ANY,&msg);
        switch(msg.type)
        {
            case MM_ALLOCATE_PAGE:
                mm_allocate_page(&msg);
                sys_send_recv(NR_SEND,msg.src,&msg);
                break;

            case MM_FREE_PAGE:
                mm_free_page(&msg);
                break;

            case MM_READ_PROG_ADDR:
                mm_read_prog_addr(&msg);
                sys_send_recv(NR_SEND,msg.src,&msg);
                break;
            case MM_EXIT:
                mm_exit(&msg);
                break;
            default:
                break;
        }
    }
}