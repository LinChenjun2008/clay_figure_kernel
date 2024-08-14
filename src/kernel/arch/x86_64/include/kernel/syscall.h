#ifndef __SYSCALL_H__
#define __SYSCALL_H__

PUBLIC void syscall_init();
PUBLIC syscall_status_t ASMLINKAGE send_recv(
    uint32_t nr,
    pid_t src_dest,
    void *msg);

PUBLIC syscall_status_t ASMLINKAGE sys_send_recv(
    uint32_t nr,
    pid_t src_dest,
    message_t *msg);

PUBLIC void inform_intr(pid_t dest);

PUBLIC syscall_status_t msg_send(pid_t dest,message_t* msg);
PUBLIC syscall_status_t msg_recv(pid_t src,message_t *msg);

#endif