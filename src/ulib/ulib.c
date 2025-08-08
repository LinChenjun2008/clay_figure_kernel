#include <kernel/global.h>

#include <kernel/syscall.h>
#include <service.h>
#include <ulib.h>

PUBLIC void exit(int status)
{
    message_t msg;
    msg.type  = KERN_EXIT;
    msg.m1.i1 = status;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    // Never return
    return;
}

PUBLIC int get_pid(void)
{
    message_t msg;
    msg.type = KERN_GET_PID;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    return msg.m1.i1;
}

PUBLIC int get_ppid(void)
{
    message_t msg;
    msg.type = KERN_GET_PPID;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    return msg.m1.i1;
}

PUBLIC int create_process(const char *name, void *proc)
{
    message_t msg;
    msg.type  = KERN_CREATE_PROC;
    msg.m3.p1 = (void *)name;
    msg.m3.p2 = proc;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    return msg.m1.i1;
}

PUBLIC void *allocate_page(void)
{
    message_t msg;
    msg.type = KERN_ALLOCATE_PAGE;
    syscall_status_t status;
    status = send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    if (status != SYSCALL_SUCCESS)
    {
        return NULL;
    }
    return msg.m2.p1;
}

PUBLIC void free_page(void *addr)
{
    message_t msg;
    msg.type  = KERN_FREE_PAGE;
    msg.m2.p1 = addr;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    return;
}

PUBLIC void read_task_addr(pid_t pid, void *addr, size_t size, void *buffer)
{
    message_t msg;
    msg.type  = KERN_READ_TASK_MEM;
    msg.m3.i1 = pid;
    msg.m3.p1 = addr;
    msg.m3.l1 = size;
    msg.m3.p2 = buffer;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    return;
}

PUBLIC uint64_t get_ticks(void)
{
    message_t msg;
    msg.type = TICK_GET_TICKS;
    send_recv(NR_BOTH, TICK, &msg);
    return msg.m3.l1;
}

PUBLIC void fill(
    void    *buffer,
    size_t   buffer_size,
    uint32_t xsize,
    uint32_t ysize,
    uint32_t x,
    uint32_t y
)
{
    message_t msg;
    msg.type  = VIEW_FILL;
    msg.m3.p1 = buffer;
    msg.m3.l1 = buffer_size;
    msg.m3.i1 = xsize;
    msg.m3.i2 = ysize;

    msg.m3.i3 = x;
    msg.m3.i4 = y;
    send_recv(NR_BOTH, VIEW, &msg);
    return;
}