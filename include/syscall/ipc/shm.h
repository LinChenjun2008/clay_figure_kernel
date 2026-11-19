// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_IPC_SHM_H__
#define __SYSCALL_IPC_SHM_H__

#include <asm/page.h>

#include <kobject.h>
#include <sync/spinlock.h>
#include <types.h>

#define IPC_PRIVATE 0

#define IPC_CREAT  01000
#define IPC_EXCL   02000
#define IPC_NOWAIT 04000

#define SHM_RDONLY 010000 // 只读附着

#define SHMMNI 512
#define SHMMAX (2048 * PG_SIZE) // 单个段的最大大小(字节)

// 用于在shm_table中标记一块共享内存段
struct shm_seg
{
    struct kobject kobj;

    phys_addr_t *phys;  // 物理页地址数组
    size_t       pages; // 页长度(phys[]项数)

    int   key;    // 查找/操作shm的依据
    int   mode;   // 访问模式
    pid_t cpid;   // 创建者pid
    int   nattch; // 附加次数
};

// 内核级共享段表
struct shm_table
{
    struct spinlock lock;
    struct shm_seg *slot[SHMMNI];
    int             gen[SHMMNI];
};

void      shm_init(void);
int       create_shm_seg(size_t size, int key, uint32_t mode);
int       shm_get(size_t size, int key, int shmflg);
uintptr_t shm_at(int shmid, uintptr_t shmaddr, int shmflg);

#endif /* __SYSCALL_IPC_SHM_H__ */