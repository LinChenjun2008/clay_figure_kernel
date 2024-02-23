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
    uintptr_t vaddr;
    task_struct_t *src = pid2task(msg->src);
    paddr = alloc_physical_page(msg->m1.i1);
    if (paddr == NULL)
    {
        msg->m2.p1 = NULL;
        return;
    }
    vaddr = (uintptr_t)paddr;
    if (src->page_dir != NULL)
    {
        vaddr = allocate_units(&src->vaddr_table,msg->m1.i1) * PG_SIZE;
        page_map(src->page_dir,paddr,(void*)vaddr);
    }
    msg->m2.p1 = (void*)vaddr;
    return;
}

PRIVATE void mm_free_page(message_t *msg)
{
    task_struct_t *src = pid2task(msg->src);
    void *vaddr = msg->m3.p1;
    void *paddr = to_physical_address(src->page_dir,vaddr);
    free_physical_page(paddr,msg->m3.i1);
    free_units(&src->vaddr_table,(uintptr_t)vaddr & ~(PG_SIZE - 1),msg->m3.i1);
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