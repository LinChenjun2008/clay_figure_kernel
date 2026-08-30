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
#include <sysinfo.h>
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
    init_free_table(&vm->copy_on_write, 8);
    return;
}

static int copy_vm_struct(struct vm_struct *dst, struct vm_struct *src)
{
    int ret = 0;
    // mapped 在copy_pg_struct时已加入copy_on_write
    ret += copy_free_table(&dst->vm_table, &src->vm_table);
    ret += copy_free_table(&dst->unmapped, &src->unmapped);
    return ret;
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
    destroy_free_table(&vm->copy_on_write);
    return;
}

static void init_pg_struct(struct pg_struct *pg)
{
    init_list(&pg->list);
    return;
}

struct copy_pg_pack
{
    struct task *dst;
    struct task *src;
};

static int traversal_copy_pg(struct list_node *node, void *arg)
{
    struct copy_pg_pack *pack = arg;
    struct task         *dst  = pack->dst;
    struct task         *src  = pack->src;
    struct list         *list = &dst->mm->pg_map.list;

    struct page_struct *src_pg = CONTAINER_OF(struct page_struct, node, node);

    struct page_struct *new_pg = kmalloc(sizeof(*new_pg), 0, 0);
    if (new_pg == NULL)
    {
        return 1;
    }
    new_pg->pfn  = src_pg->pfn;
    new_pg->virt = src_pg->virt;
    list_append(list, &new_pg->node);
    page_reference_inc(new_pg->pfn);

    void *new_page = (void *)PFN_TO_ADDR(new_pg->pfn);
    void *src_page = (void *)PFN_TO_ADDR(src_pg->pfn);
    // 加入cow表
    mm_map_copy_on_write(dst, new_page, new_pg->virt);
    mm_map_copy_on_write(src, src_page, src_pg->virt);
    return 0; // 返回0使list_traversal继续遍历
}

static int copy_pg_struct(struct task *dst, struct task *src)
{
    struct list *src_list = &src->mm->pg_map.list;

    struct copy_pg_pack pack = { dst, src };
    // 遍历src->list,将其中的page_struct复制到dst->list中
    // 若list_traversal返回非NULL值则代表复制出错.
    return list_traversal(src_list, traversal_copy_pg, &pack) == NULL;
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
        void               *addr = PHYS_TO_VIRT(PFN_TO_ADDR(page->pfn));
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

int copy_mm_struct(struct task *dst, struct task *src)
{
    // copy pg_struct
    if (!copy_pg_struct(dst, src))
    {
        return -1;
    }

    // copy vm_struct
    if (copy_vm_struct(&dst->mm->vm_map, &src->mm->vm_map) < 0)
    {
        return -1;
    }
    return 0;
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
    size_t    size  = pages << PG_SIZE_SHIFT;
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
    size_t pfn = ADDR_TO_PFN((uintptr_t)phys);

    struct page_struct *page_struct = NULL;
    page_struct = CONTAINER_OF(struct page_struct, node, node);
    return page_struct->pfn == pfn;
}

static struct page_struct *get_page_by_phys(struct pg_struct *pg, void *phys)
{
    struct list_node *node;
    node = list_traversal(&pg->list, traversal_by_phys, phys);
    if (node == NULL)
    {
        return NULL;
    }
    return CONTAINER_OF(struct page_struct, node, node);
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
    node = list_traversal(&pg->list, traversal_by_virt, virt);
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

// 将页设为cow,fork时使用
void mm_map_copy_on_write(struct task *task, void *phys, void *virt)
{
    ASSERT(task->mm != NULL && phys != NULL && virt != NULL);

    struct vm_struct *vm = &task->mm->vm_map;

    // 移除旧表中的页(如果有)
    if (free_table_find(&vm->mapped, (uintptr_t)virt))
    {
        free_table_remove(&vm->mapped, (uintptr_t)virt, PG_SIZE);
    }
    if (free_table_find(&vm->unmapped, (uintptr_t)virt))
    {
        free_table_remove(&vm->unmapped, (uintptr_t)virt, PG_SIZE);
    }
    // 加入写时复制表
    free_table_add(&vm->copy_on_write, (uintptr_t)virt, PG_SIZE);

    // 设置页表中的标志
    arch_mm_copy_on_write(task, phys, virt);
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
        if (page_struct == NULL)
        {
            start += PG_SIZE;
            continue;
        }
        list_remove(&page_struct->node);

        start += PG_SIZE;
        mapped_pages--;
        uintptr_t page_address = PFN_TO_ADDR(page_struct->pfn);
        free_a_page(PHYS_TO_VIRT(page_address));
        kfree(page_struct);
    }
    return;
}

void *mm_allocate_a_page_lock(void)
{
    struct task *task = get_current_task();
    ASSERT(task->mm != NULL);

    struct pg_struct *pg = &task->mm->pg_map;

    struct page_struct *page_struct = kmalloc(sizeof(*page_struct), 0, 0);
    if (page_struct == NULL)
    {
        return NULL;
    }

    void *addr = allocate_a_page_lock();
    if (addr == NULL)
    {
        kfree(page_struct);
        return NULL;
    }
    page_struct->pfn  = ADDR_TO_PFN((uintptr_t)VIRT_TO_PHYS(addr));
    page_struct->virt = NULL;
    list_append(&pg->list, &page_struct->node);
    return VIRT_TO_PHYS(addr);
}

void *mm_allocate_a_page(void)
{
    page_mgr_lock();
    void *ret = mm_allocate_a_page_lock();
    page_mgr_unlock();
    return ret;
}

size_t mm_free_a_page_lock(void *addr, size_t pfn)
{
    struct task *task = get_current_task();
    ASSERT(task->mm != NULL);

    struct pg_struct *pg = &task->mm->pg_map;

    struct page_struct *page_struct = get_page_by_phys(pg, addr);
    if (page_struct == NULL)
    {
        return 0;
    }

    list_remove(&page_struct->node);
    kfree(page_struct);

    return free_a_page_lock(pfn);
}

size_t mm_free_a_page(void *addr)
{
    size_t pfn = ADDR_TO_PFN((uintptr_t)VIRT_TO_PHYS(addr));

    page_mgr_lock();
    page_struct_lock(pfn);
    size_t ret = mm_free_a_page_lock(addr, pfn);
    page_struct_unlock(pfn);
    page_mgr_unlock();
    return ret;
}