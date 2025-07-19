// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define NR_SEND 0x80000001
#define NR_RECV 0x80000002
#define NR_BOTH 0x80000003

#define RECV_FROM_INT -1
#define RECV_FROM_ANY -2

#define SYSCALL_SUCCESS        0x80000000
#define SYSCALL_ERROR          0xc0000000
#define SYSCALL_NO_SYSCALL     (SYSCALL_ERROR | 0x00000001)
#define SYSCALL_DEADLOCK       (SYSCALL_ERROR | 0x00000002)
#define SYSCALL_DEST_NOT_EXIST (SYSCALL_ERROR | 0x00000003)
#define SYSCALL_SRC_NOT_EXIST  (SYSCALL_ERROR | 0x00000004)

typedef struct message_s
{
    volatile pid_t    src;
    volatile uint32_t type;
    union
    {
        struct
        {
            uint32_t i1;
            uint32_t i2;
            uint32_t i3;
            uint32_t i4;
        } m1;

        struct
        {
            void *p1;
            void *p2;
            void *p3;
            void *p4;
        } m2;

        struct
        {
            uint32_t i1;
            uint32_t i2;
            uint32_t i3;
            uint32_t i4;
            uint64_t l1;
            uint64_t l2;
            void    *p1;
            void    *p2;
        } m3;
    };
} message_t;

typedef uint32_t syscall_status_t;

PUBLIC void syscall_init(void);

PUBLIC syscall_status_t ASMLINKAGE
send_recv(uint32_t nr, pid_t src_dest, void *msg);
PUBLIC syscall_status_t ASMLINKAGE
sys_send_recv(uint32_t nr, pid_t src_dest, message_t *msg);

PUBLIC void inform_intr(pid_t dest);

PUBLIC syscall_status_t msg_send(pid_t dest, message_t *msg);
PUBLIC syscall_status_t msg_recv(pid_t src, message_t *msg);

#endif