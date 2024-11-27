/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include <lib/list.h>
#include <sync/atomic.h>

typedef struct task_struct_s task_struct_t;

typedef struct semaphore_s
{
    atomic_t          value;
    list_t            waiters;
    task_struct_t    *holder;
    uint64_t          holder_repeat_nr;
} semaphore_t;

PUBLIC void init_semaphore(semaphore_t *sema,uint32_t value);
PUBLIC status_t sema_free(semaphore_t *sema);
PUBLIC status_t sema_down(semaphore_t *sema);
PUBLIC status_t sema_up(semaphore_t *sema);

#endif