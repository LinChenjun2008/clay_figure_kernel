#ifndef __SYSCALL_H__
#define __SYSCALL_H__

PUBLIC void syscall_init();
PUBLIC void syscall(uint32_t nr,pid_t src_dest,void *msg);

#endif