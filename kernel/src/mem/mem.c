// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <lib/free_table.h>
#include <mem.h>
#include <print.h>
#include <task.h>

void mem_init(struct system_info *system_info)
{
    printk("mem_init: page management initializing...\n");
    page_mgr_init(system_info);
    printk("mem_init: memory management initializing...\n");
    mem_allocator_init();
    return;
}

static void init_vm_struct(struct vm_struct *vm)
{
    init_free_table(&vm->vm_table, 8);
    init_free_table(&vm->mapped, 8);
    init_free_table(&vm->unmapped, 8);
    return;
}

static void destory_vm_struct(struct vm_struct *vm)
{
    destroy_free_table(&vm->vm_table);
    destroy_free_table(&vm->mapped);
    destroy_free_table(&vm->unmapped);
    return;
}

static void init_pg_struct(struct pg_struct *pg)
{
    init_list(&pg->list);
    return;
}

static void destory_pg_struct(struct pg_struct *pg)
{
    ASSERT(list_len(&pg->list) == 0);
    return;
}

struct mm_struct *allocate_mm_struct(void)
{
    struct mm_struct *mm = kmalloc(sizeof(*mm), 0, 0);
    if (mm == NULL)
    {
        return NULL;
    }
    init_vm_struct(&mm->vm_map);
    init_pg_struct(&mm->pg_map);
    return mm;
}

void destory_mm_struct(struct mm_struct *mm)
{
    destory_vm_struct(&mm->vm_map);
    destory_pg_struct(&mm->pg_map);
    kfree(mm);
    return;
}

void *sys_mmap(void *addr, size_t pages, uint64_t flags)
{
    struct task *task = get_current_task();

    uintptr_t start = (uintptr_t)addr;
    size_t    size  = pages << PAGE_SIZE_SHIFT;
    if (addr != NULL && flags & MAP_FIXED)
    {
        if (free_table_remove(&task->mm->vm_map.vm_table, start, size) < 0)
        {
            return NULL;
        }
        free_table_add(&task->mm->vm_map.unmapped, start, size);
        return addr;
    }

    start = free_table_allocate(&task->mm->vm_map.vm_table, size);
    if (start == -1UL)
    {
        return NULL;
    }
    free_table_add(&task->mm->vm_map.unmapped, start, size);
    return (void *)start;
}