#include <kernel/global.h>
#include <kernel/syscall.h>
#include <task/task.h>
#include <service.h>
#include <mem/mem.h>
#include <lib/alloc_table.h>

#include <log.h>

PRIVATE void mm_allocate_page(message_t *msg)
{
    void *paddr;
    uintptr_t vaddr_start,vaddr;
    task_struct_t *src = pid2task(msg->src);
    if (src->page_dir == NULL)
    {
        msg->m2.p1 = NULL;
        return;
    }

    vaddr_start = allocate_units(&src->vaddr_table,msg->m1.i1);
    if (ERROR((int64_t)vaddr_start))
    {
        msg->m2.p1 = NULL;
        return;
    }
    vaddr_start *= PG_SIZE;
    vaddr = vaddr_start;
    uint32_t i;
    for (i = 0;i < msg->m1.i1;i++)
    {
        paddr = alloc_physical_page(1);
        if (paddr == NULL)
        {
            while (i > 0)
            {
                vaddr -= PG_SIZE;
                page_unmap(src->page_dir,(void*)vaddr);
                i--;
            }
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
    uintptr_t vaddr = (uintptr_t)msg->m3.p1;
    free_units(&src->vaddr_table,(uintptr_t)vaddr & ~(PG_SIZE - 1),msg->m3.i1);

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
        pr_log("\3 read prog addr failed. %d\n",msg->m1.i1);
        return;
    }
    uint8_t *buffer = to_physical_address(src->page_dir,msg->m3.p2);
    if (buffer == NULL)
    {
        msg->m1.i1 = 2;
        pr_log("\3 read prog addr failed. %d\n",msg->m1.i1);
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
        pr_log("\3 read prog addr failed. %d\n",msg->m1.i1);
        return;
    }
    data = KADDR_P2V(data);
    memcpy(buffer,data,msg->m3.l1);
    msg->m1.i1 = SYSCALL_SUCCESS;
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
            default:
                break;
        }
    }
}