// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/intr/handler.h>

#include <lib/free_table.h>
#include <mem.h>
#include <mem/allocator.h>
#include <mem/page.h>
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
    if (vm == NULL)
    {
        return;
    }
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
        free_a_page(addr);
        kfree(page);
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

void *mm_allocate_address(void *addr, size_t pages)
{
    struct task      *task = get_current_task();
    struct vm_struct *vm   = &task->mm->vm_map;

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

static int traversal_by_phys(struct list_node *node, void *phys)
{
    size_t pfn = (uintptr_t)phys >> PAGE_SIZE_SHIFT;

    struct page_struct *page_struct = NULL;
    page_struct = CONTAINER_OF(struct page_struct, node, node);
    return page_struct->pfn == pfn;
}

static int traversal_by_virt(struct list_node *node, void *virt)
{
    struct page_struct *page_struct = NULL;
    page_struct = CONTAINER_OF(struct page_struct, node, node);
    return page_struct->virt == virt;
}

static struct page_struct *get_page_by_virt(struct pg_struct *pg, void *virt)
{
    struct list_node *node;
    node = list_traversal_remove(&pg->list, traversal_by_virt, virt);
    if (node == NULL)
    {
        return NULL;
    }
    return CONTAINER_OF(struct page_struct, node, node);
}

void mm_map(struct task *task, void *phys, void *virt)
{
    ASSERT(task->mm != NULL && phys != NULL && virt != NULL);

    struct vm_struct *vm = &task->mm->vm_map;
    struct pg_struct *pg = &task->mm->pg_map;

    free_table_remove(&vm->unmapped, (uintptr_t)virt, PG_SIZE);
    free_table_add(&vm->mapped, (uintptr_t)virt, PG_SIZE);

    struct list_node *node;
    node = list_traversal(&pg->list, traversal_by_phys, phys);
    ASSERT(node != NULL);

    struct page_struct *page_struct = NULL;
    page_struct       = CONTAINER_OF(struct page_struct, node, node);
    page_struct->virt = virt;

    arch_mm_map(task, phys, virt);
    return;
}

void mm_free_address(void *addr, size_t pages)
{
    if (addr == NULL)
    {
        return;
    }
    struct task      *task = get_current_task();
    struct vm_struct *vm   = &task->mm->vm_map;
    struct pg_struct *pg   = &task->mm->pg_map;

    uintptr_t start = (uintptr_t)addr;

    struct free_table *table = NULL;

    size_t mapped_pages = 0;
    size_t remain_pages;
    for (remain_pages = pages; remain_pages > 0; remain_pages--)
    {
        table = NULL;
        if (free_table_find(&vm->mapped, start))
        {
            table = &vm->mapped;
            mapped_pages++;
        }
        if (free_table_find(&vm->unmapped, start))
        {
            table = &vm->unmapped;
        }
        if (table == NULL)
        {
            start += PG_SIZE;
            continue;
        }

        free_table_remove(table, start, PG_SIZE);
        free_table_add(&vm->vm_table, start, PG_SIZE);
        start += PG_SIZE;
    }

    // 释放物理页
    start = (uintptr_t)addr;
    while (mapped_pages > 0)
    {
        struct page_struct *page_struct = get_page_by_virt(pg, (void *)start);

        // 未分配
        if (page_struct == NULL)
        {
            start += PG_SIZE;
            continue;
        }
        start += PG_SIZE;
        mapped_pages--;
        uintptr_t page_address = page_struct->pfn << PAGE_SIZE_SHIFT;
        free_a_page(PHYS_TO_VIRT(page_address));
        kfree(page_struct);
    }
    return;
}

void *mm_allocate_a_page(void)
{
    struct task *task = get_current_task();
    ASSERT(task->mm != NULL);

    struct pg_struct *pg = &task->mm->pg_map;

    void *addr = allocate_a_page();
    if (addr == NULL)
    {
        return NULL;
    }

    struct page_struct *page_struct = kmalloc(sizeof(*page_struct), 0, 0);
    if (page_struct == NULL)
    {
        free_a_page(addr);
        return NULL;
    }
    page_struct->pfn  = (uintptr_t)VIRT_TO_PHYS(addr) >> PAGE_SIZE_SHIFT;
    page_struct->virt = NULL;
    list_append(&pg->list, &page_struct->node);
    return VIRT_TO_PHYS(addr);
}
