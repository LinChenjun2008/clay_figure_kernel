#include <kernel/global.h>

#include <kernel/syscall.h>
#include <service.h>
#include <std/string.h>
#include <ulib.h>

PUBLIC void exit(int status)
{
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type                   = KERN_EXIT;
    msg.m[IN_KERN_EXIT_STATUS] = status;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    // Never return
    return;
}

PUBLIC int get_pid(void)
{
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = KERN_GET_PID;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    return (pid_t)msg.m[OUT_KERN_GET_PID_PID];
}

PUBLIC int get_ppid(void)
{
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = KERN_GET_PPID;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    return (pid_t)msg.m[OUT_KERN_GET_PPID_PPID];
}

PUBLIC int create_process(const char *name, void *proc)
{
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type                        = KERN_CREATE_PROC;
    msg.m[IN_KERN_CREATE_PROC_NAME] = (uint64_t)name;
    msg.m[IN_KERN_CREATE_PROC_PROC] = (uint64_t)proc;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    return msg.m[OUT_KERN_CREATE_PROC_PID];
}

PUBLIC pid_t waitpid(pid_t pid, int *status, int options)
{
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type                   = KERN_WAITPID;
    msg.m[IN_KERN_WAITPID_PID] = (uint64_t)pid;
    msg.m[IN_KERN_WAITPID_OPT] = options;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    *status = (int)msg.m[OUT_KERN_WAITPID_STATUS];
    return (pid_t)msg.m[OUT_KERN_WAITPID_PID];
}

PUBLIC void *allocate_page(void)
{
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = KERN_ALLOCATE_PAGE;
    syscall_status_t status;
    status = send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    if (status != SYSCALL_SUCCESS)
    {
        return NULL;
    }
    return (void *)msg.m[OUT_KERN_ALLOCATE_PAGE_ADDR];
}

PUBLIC void free_page(void *addr)
{
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type                      = KERN_FREE_PAGE;
    msg.m[IN_KERN_FREE_PAGE_ADDR] = (uint64_t)addr;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    return;
}

PUBLIC void read_task_addr(pid_t pid, void *addr, size_t size, void *buffer)
{
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type                            = KERN_READ_TASK_MEM;
    msg.m[IN_KERN_READ_TASK_MEM_PID]    = pid;
    msg.m[IN_KERN_READ_TASK_MEM_ADDR]   = (uint64_t)addr;
    msg.m[IN_KERN_READ_TASK_MEM_SIZE]   = size;
    msg.m[IN_KERN_READ_TASK_MEM_BUFFER] = (uint64_t)buffer;
    send_recv(NR_SEND, SEND_TO_KERNEL, &msg);
    return;
}

PUBLIC uint64_t get_ticks(void)
{
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = TICK_GET_TICKS;
    send_recv(NR_BOTH, TICK, &msg);
    return msg.m[OUT_TICK_GET_TICKS_TICKS];
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
    memset(&msg, 0, sizeof(msg));
    msg.type                        = VIEW_FILL;
    msg.m[IN_VIEW_FILL_BUFFER]      = (uint64_t)buffer;
    msg.m[IN_VIER_FILL_BUFFER_SIZE] = buffer_size;
    msg.m[IN_VIEW_FILL_XSIZE]       = xsize;
    msg.m[IN_VIEW_FILL_YSIZE]       = ysize;

    msg.m[IN_VIEW_FILL_X] = x;
    msg.m[IN_VIEW_FILL_Y] = y;
    send_recv(NR_BOTH, VIEW, &msg);
    return;
}