// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
#include <asm/mem/page.h>
#include <asm/task.h>

#include <mem/allocator.h>
#include <mm_struct.h>
#include <std/string.h>
#include <task.h>

static void kernel_process(void *func)
{
    intr_disable();

    struct task *curr_task = get_current_task();

    size_t ustack_size = curr_task->ustack_pages * PG_SIZE;
    allocate_pages(curr_task->ustack_pages, (void **)&curr_task->ustack_base);

    uint64_t *page_table   = curr_task->page_dir;
    void     *ustack       = VIRT_TO_PHYS(curr_task->ustack_base);
    void     *ustack_vaddr = (void *)(USER_STACK_VADDR_TOP - ustack_size);
    page_map(page_table, ustack, ustack_vaddr, curr_task->ustack_pages);
    task_page_table_active(curr_task);

    switch_to_user(func);
    return;
}

static uint64_t *create_page_table(void)
{
    uint64_t *page_table = NULL;
    allocate_pages(1, (void **)&page_table);

    uint64_t        *kernel_page_table;
    struct task_man *task_man = get_current_task()->cpu->task_man;
    kernel_page_table         = PHYS_TO_VIRT(task_man->kernel_page_table_pos);

    memset(page_table, 0, PT_SIZE);
    memcpy(page_table + 0x100, kernel_page_table + 0x100, PT_SIZE / 2);
    return VIRT_TO_PHYS(page_table);
}

static void user_vaddr_table_init(struct task *task)
{
    size_t   block_size   = sizeof(*task->mm_alloc.blocks);
    uint64_t total_blocks = 256;
    void    *blocks;
    kmalloc(block_size * total_blocks, 0, 0, &blocks);
    init_mm_struct(&task->mm_alloc, blocks, total_blocks);

    kmalloc(block_size * total_blocks, 0, 0, &blocks);
    init_mm_struct(&task->mm_using, blocks, total_blocks);

    kmalloc(block_size * total_blocks, 0, 0, &blocks);
    init_mm_struct(&task->mm_pages, blocks, total_blocks);

    uintptr_t vm_start = USER_VADDR_START;

    uintptr_t ustack_vaddr_base;
    ustack_vaddr_base = (USER_STACK_VADDR_TOP - task->ustack_pages * PG_SIZE);
    size_t vm_size    = (ustack_vaddr_base - USER_VADDR_START);
    mm_add(&task->mm_alloc, vm_start, vm_size);
    return;
}

struct task *process_execute(
    const char *name,
    uint64_t    prio,
    size_t      kstack_pages,
    size_t      ustack_pages,
    void       *proc
)
{
    struct task *task = allocate_task_struct();
    uintptr_t    kstack_base;
    allocate_pages(kstack_pages, (void **)&kstack_base);
    init_task_struct(task, name, prio, kstack_base, kstack_pages, ustack_pages);
    create_task_context(task, kernel_process, proc);

    task->page_dir = create_page_table();
    user_vaddr_table_init(task);

    get_current_task()->childs++;
    cpu_task_list_insert(task->cpu, task);
    return task;
}