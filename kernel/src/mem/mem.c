// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
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
    register_handler(0x0e, page_faule);
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
    // free physical pages
    size_t i;
    size_t page_struct_count = list_len(&pg->list);

    for (i = 0; i < page_struct_count; i++)
    {
        struct list_node *node = list_pop(&pg->list);
        ASSERT(node != NULL);

        struct page_struct *page = CONTAINER_OF(struct page_struct, node, node);
        void               *addr = PHYS_TO_VIRT(page->pfn << PAGE_SIZE_SHIFT);
        free_pages(addr, 1);
    }

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

void *mm_allocate_address(struct vm_struct *vm, void *addr, size_t pages)
{
    uintptr_t start = (uintptr_t)addr;
    size_t    size  = pages << PAGE_SIZE_SHIFT;
    if (addr != NULL)
    {
        if (free_table_remove(&vm->vm_table, start, size) < 0)
        {
            return NULL;
        }
        free_table_add(&vm->unmapped, start, size);
        return addr;
    }

    start = free_table_allocate(&vm->vm_table, size);
    if (start == -1UL)
    {
        return NULL;
    }
    free_table_add(&vm->unmapped, start, size);
    return (void *)start;
}

void *mm_allocate_pages(size_t pages)
{
    struct task *task = get_current_task();
    ASSERT(task->mm != NULL);

    struct pg_struct *pg = &task->mm->pg_map;

    void *addr = allocate_pages(pages);
    if (addr == NULL)
    {
        return NULL;
    }

    struct page_struct *page_struct = kmalloc(sizeof(*page_struct), 0, 0);
    if (page_struct == NULL)
    {
        free_pages(addr, pages);
        return NULL;
    }
    page_struct->pfn = (uintptr_t)VIRT_TO_PHYS(addr) >> PAGE_SIZE_SHIFT;
    list_append(&pg->list, &page_struct->node);
    return VIRT_TO_PHYS(addr);
}

static int find_page_struct(struct list_node *node, void *arg)
{
    size_t pfn = (uintptr_t)VIRT_TO_PHYS(arg) >> PAGE_SIZE_SHIFT;

    struct page_struct *page_struct = NULL;
    page_struct = CONTAINER_OF(struct page_struct, node, node);
    return page_struct->pfn == pfn;
}

void mm_free_pages(void *addr, size_t pages)
{
    struct task *task = get_current_task();
    ASSERT(task->mm != NULL);

    struct pg_struct *pg = &task->mm->pg_map;

    struct list_node *node;
    node = list_traversal_remove(&pg->list, find_page_struct, addr);
    ASSERT(node != NULL);

    struct page_struct *page_struct = NULL;
    page_struct = CONTAINER_OF(struct page_struct, node, node);
    free_pages(addr, pages);
    kfree(page_struct);
    return;
}