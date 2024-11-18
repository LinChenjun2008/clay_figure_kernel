/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU Lesser General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 宽通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 宽通用公共许可证，了解详情。

你应该随程序获得一份 GNU 宽通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/global.h>
#include <task/task.h>      // task struct & functions,spinlock
#include <mem/mem.h>        // alloc_physical_page,page_map,pmalloc,pfree
#include <intr.h>           // intr_stack_t
#include <io.h>             // set_cr3
#include <device/cpu.h>     // apic_id
#include <std/string.h>     // memset,memcpy
#include <kernel/syscall.h> // sys_send_recv
#include <service.h>        // MM_EXIT

#include <log.h>

extern taskmgr_t *tm;

PUBLIC void prog_exit(addr_t func,wordsize_t arg)
{
    int ret_value = ((int (*)(void*))func)((void*)arg);

    message_t msg;
    msg.type = MM_EXIT;
    msg.m1.i1 = ret_value;
    send_recv(NR_SEND,MM,&msg);
    pr_log("\3 %s: Shuold not be here.",__func__);
    while (1) continue;
}

extern void asm_process_start(void *stack);
PRIVATE void start_process(void *process)
{
    void *func = process;
    task_struct_t *cur = running_task();
    addr_t kstack = (addr_t)cur->context;
    kstack += sizeof(task_context_t);

    addr_t ustack;
    status_t status = alloc_physical_page(1,&ustack);
    if (ERROR(status))
    {
        pr_log("\3 Alloc User Stack error.\n");
        message_t msg;
        msg.type = MM_EXIT;
        msg.m1.i1 = K_ERROR;
        sys_send_recv(NR_BOTH,MM,&msg);
        pr_log("\3 %s: Shuold not be here.",__func__);
        while (1) continue;
    }
    cur->ustack_base = ustack;
    cur->ustack_size = PG_SIZE;
    page_map(cur->page_dir,(void*)ustack,(void*)USER_STACK_VADDR_BASE);

    intr_stack_t *proc_stack = (intr_stack_t*)kstack;
    memset(proc_stack,0,sizeof(*proc_stack));
    proc_stack->r15 = 0;
    proc_stack->r14 = 0;
    proc_stack->r13 = 0;
    proc_stack->r12 = 0;
    proc_stack->r11 = 0;
    proc_stack->r10 = 0;
    proc_stack->r9 = 0;
    proc_stack->r8 = 0;

    proc_stack->rdi = (wordsize_t)func;
    proc_stack->rsi = 0;
    proc_stack->rbp = 0;
    proc_stack->rdx = 0;
    proc_stack->rcx = 0;
    proc_stack->rbx = 0;
    proc_stack->rax = 0;

    proc_stack->gs  = SELECTOR_DATA64_U;
    proc_stack->fs  = SELECTOR_DATA64_U;
    proc_stack->es  = SELECTOR_DATA64_U;
    proc_stack->ds  = SELECTOR_DATA64_U;
    proc_stack->rip = (void*)prog_exit;
    proc_stack->cs  = SELECTOR_CODE64_U;
    proc_stack->rflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->rsp = USER_STACK_VADDR_BASE + PG_SIZE;
    proc_stack->ss = SELECTOR_DATA64_U;

    asm_process_start((void*)proc_stack);
}

PRIVATE void page_dir_activate(task_struct_t *task)
{
    void *page_dir_table_pos = (void*)KERNEL_PAGE_DIR_TABLE_POS;
    if (task->page_dir != NULL)
    {
        page_dir_table_pos = task->page_dir;
    }
    set_cr3((wordsize_t)page_dir_table_pos);
    return;
}

PUBLIC void prog_activate(task_struct_t *task)
{
    page_dir_activate(task);
    if (task->page_dir != NULL)
    {
        update_tss_rsp0(task);
    }
    return;
}

PRIVATE uint64_t *create_page_dir(void)
{
    uint64_t *pgdir_v;
    status_t status = pmalloc(PT_SIZE,&pgdir_v);
    if (ERROR(status))
    {
        return NULL;
    }
    pgdir_v = KADDR_P2V(pgdir_v);
    memset(pgdir_v,0,PT_SIZE);
    memcpy(pgdir_v + 0x100,(uint64_t*)KADDR_P2V(KERNEL_PAGE_DIR_TABLE_POS)
                            + 0x100,2048);
    return (uint64_t*)KADDR_V2P(pgdir_v);
}

PRIVATE status_t user_vaddr_table_init(task_struct_t *task)
{
    size_t entry_size          = sizeof(*task->vaddr_table.entries);
    uint64_t number_of_entries = 1024;
    void *p;
    status_t status = pmalloc(entry_size * number_of_entries,&p);
    if (ERROR(status))
    {
        return status;
    }
    p = KADDR_P2V(p);
    allocate_table_init(&task->vaddr_table,p,number_of_entries);

    uint64_t index = USER_VADDR_START / PG_SIZE;
    uint64_t cnt   = (USER_STACK_VADDR_BASE - USER_VADDR_START) / PG_SIZE;
    free_units(&task->vaddr_table,index,cnt);
    return K_SUCCESS;
}

PUBLIC task_struct_t *prog_execute(
    const char *name,
    uint64_t priority,
    size_t kstack_size,
    void *prog)
{
    ASSERT(!(kstack_size & (kstack_size - 1)));
    status_t status;
    pid_t pid;
    status = task_alloc(&pid);
    ASSERT(!ERROR(status));
    if (ERROR(status))
    {
        return NULL;
    }
    void *kstack_base;
    status = pmalloc(kstack_size,&kstack_base);
    ASSERT(!ERROR(status));
    if (ERROR(status))
    {
        task_free(pid);
        return NULL;
    }
    task_struct_t *task = pid2task(pid);
    init_task_struct(task,
                     name,priority,
                     (addr_t)KADDR_P2V(kstack_base),
                     kstack_size);
    create_task_struct(task,start_process,(uint64_t)prog);
    task->page_dir = create_page_dir();
    ASSERT(task->page_dir != NULL);
    if (task->page_dir == NULL)
    {
        pr_log("\3 Can not alloc memory for task page table.\n");
        pfree(kstack_base);
        task_free(pid);
        return NULL;
    }
    status = user_vaddr_table_init(task);
    ASSERT(!ERROR(status));
    if (ERROR(status))
    {
        pr_log("\3 Can not init vaddr table.\n");
        pfree(kstack_base);
        pfree(task->page_dir);
        task_free(pid);
        return NULL;
    }
    spinlock_lock(&tm->core[apic_id()].task_list_lock);
    task_list_insert(&tm->core[apic_id()].task_list,task);
    spinlock_unlock(&tm->core[apic_id()].task_list_lock);
    return task;
}