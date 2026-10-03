// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/intr/handler.h>
#include <asm/page.h>

#include <errno.h>
#include <lib/free_table.h>
#include <mem.h>
#include <mem/allocator.h>
#include <mem/page.h>
#include <mem/struct.h>
#include <panic.h>
#include <print.h>
#include <std/string.h>
#include <sysinfo.h>
#include <task.h>
#include <task/struct.h>

void mem_init(struct system_info *system_info)
{
    printk("mem_init: page management initializing...\n");
    page_mgr_init(system_info);
    printk("mem_init: memory management initializing...\n");
    mem_allocator_init();
    register_handler(0x0e, page_fault);
    return;
}

static void init_vm_struct(struct vm_struct *vm)
{
    init_free_table(&vm->table, 8);
    return;
}

// dst 必须是空表
static int copy_vm_struct(struct vm_struct *dst, struct vm_struct *src)
{
    return copy_free_table(&dst->table, &src->table);
}

static void destroy_vm_struct(struct vm_struct *vm)
{
    if (vm == NULL)
    {
        return;
    }
    destroy_free_table(&vm->table);
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

// 复制页(COW)
static int traversal_copy_pg(struct list_node *node, void *arg)
{
    struct copy_pg_pack *pack = arg;
    struct task         *dst  = pack->dst;
    struct task         *src  = pack->src;
    struct list         *list = &dst->mm->pg_map.list;

    struct page_struct *src_pg = CONTAINER_OF(struct page_struct, node, node);

    // 创建新的页结构
    struct page_struct *new_pg = kmalloc(sizeof(*new_pg), 0, 0);
    if (new_pg == NULL)
    {
        return 1;
    }
    new_pg->pfn  = src_pg->pfn;
    new_pg->virt = src_pg->virt;
    list_append(list, &new_pg->node);
    page_reference_inc(src_pg->pfn);

    phys_addr_t new_page = PFN_TO_ADDR(new_pg->pfn);
    phys_addr_t src_page = PFN_TO_ADDR(src_pg->pfn);
    // 加入cow表
    if (mm_map_cow(dst, new_page, new_pg->virt) < 0)
    {
        return 1;
    }
    if (mm_map_cow(src, src_page, src_pg->virt) < 0)
    {
        return 1;
    }
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

static void destroy_pg_struct(struct pg_struct *pg)
{
    if (pg == NULL)
    {
        return;
    }
    // free physical pages
    size_t i;
    size_t page_struct_count = list_len(&pg->list);

    for (i = 0; i < page_struct_count; i++)
    {
        struct list_node *node = list_pop(&pg->list);
        ASSERT(node != NULL);

        struct page_struct *page = CONTAINER_OF(struct page_struct, node, node);
        phys_addr_t         addr = PFN_TO_ADDR(page->pfn);
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
    int ret = copy_vm_struct(&dst->mm->vm_map, &src->mm->vm_map);
    if (ret < 0)
    {
        return ret;
    }

    // copy pg_struct
    // 遍历src的页结构,为dst建立页结构,并把双方共享的页转为COW
    if (!copy_pg_struct(dst, src))
    {
        return -ENOMEM;
    }
    return 0;
}

void destroy_mm_struct(struct mm_struct *mm)
{
    if (mm == NULL)
    {
        return;
    }
    destroy_vm_struct(&mm->vm_map);
    destroy_pg_struct(&mm->pg_map);
    kfree(mm);
    return;
}

static int is_user_address(uintptr_t addr, size_t size)
{
    if ((addr & (PG_SIZE - 1)) != 0)
    {
        return 0;
    }
    if (!USER_VMA_SPACE(addr))
    {
        return 0;
    }
    if (size > USER_VMA_TOP - addr)
    {
        return 0;
    }
    return 1;
}

uintptr_t mm_allocate_address(uintptr_t addr, size_t pages)
{
    if (pages == 0 || pages > ((size_t)-1 >> PG_SIZE_SHIFT))
    {
        return 0;
    }
    size_t size = pages << PG_SIZE_SHIFT;

    if (addr != 0 && !is_user_address(addr, size))
    {
        return 0;
    }

    struct task *task = get_current_task();
    if (task->mm == NULL)
    {
        return 0;
    }
    struct vm_struct *vm = &task->mm->vm_map;
    uintptr_t         start;

    if (addr != 0)
    {
        // 未分配 -> 已分配但未映射
        start = addr;
        if (ft_remove(&vm->table, start, size, VM_FRE) < 0)
        {
            return 0;
        }
    }
    else
    {
        start = ft_allocate(&vm->table, size, VM_FRE);
        if ((intptr_t)start < 0)
        {
            return 0;
        }
    }

    // 已分配但未映射
    int add_status = ft_add(&vm->table, start, size, VM_UMP);
    if (add_status < 0)
    {
        int back_status = ft_add(&vm->table, start, size, VM_FRE);
        if (back_status < 0)
        {
            PANIC("mm_allocate_address: Cannot roll back the range.");
        }
        return 0;
    }
    return start;
}

static int traversal_by_phys(struct list_node *node, void *arg)
{
    phys_addr_t phys = *(phys_addr_t *)arg;
    size_t      pfn  = ADDR_TO_PFN(phys);

    struct page_struct *page_struct = NULL;
    page_struct = CONTAINER_OF(struct page_struct, node, node);
    return page_struct->pfn == pfn;
}

static struct page_struct *
get_page_by_phys(struct pg_struct *pg, phys_addr_t phys)
{
    struct list_node *node;
    node = list_traversal(&pg->list, traversal_by_phys, &phys);
    if (node == NULL)
    {
        return NULL;
    }
    return CONTAINER_OF(struct page_struct, node, node);
}

static int traversal_by_virt(struct list_node *node, void *arg)
{
    uintptr_t virt = *(uintptr_t *)arg;

    struct page_struct *page_struct = NULL;
    page_struct = CONTAINER_OF(struct page_struct, node, node);
    return page_struct->virt == virt;
}

static struct page_struct *
get_page_by_virt(struct pg_struct *pg, uintptr_t virt)
{
    struct list_node *node;
    node = list_traversal(&pg->list, traversal_by_virt, &virt);
    if (node == NULL)
    {
        return NULL;
    }
    return CONTAINER_OF(struct page_struct, node, node);
}

int mm_check_addr(struct task *task, void *addr, size_t size, int flags)
{
    if (size == 0)
    {
        return 0;
    }
    ASSERT(IS_USER_TASK(task) && task->mm != NULL);

    uintptr_t start = (uintptr_t)addr;
    uintptr_t end   = start + size - 1;
    if (end < start)
    {
        return -EINVAL;
    }
    if (!USER_VMA_SPACE(start) || !USER_VMA_SPACE(end))
    {
        return -EINVAL;
    }
    struct vm_struct *vm   = &task->mm->vm_map;
    uintptr_t         page = start & ~(PG_SIZE - 1);

    while (page <= (end & ~(PG_SIZE - 1)))
    {
        int mapped   = ft_find(&vm->table, page, VM_MAP);
        int unmapped = ft_find(&vm->table, page, VM_UMP);
        int cow      = ft_find(&vm->table, page, VM_COW);

        // 页必须已分配给该任务
        if (!mapped && !unmapped && !cow)
        {
            return -EFAULT; // 地址未分配
        }
        // 要求已映射时, 不允许懒分配页
        if (flags == ADDR_MAPPED && !mapped)
        {
            return -EFAULT;
        }
        page += PG_SIZE;
    }
    return 0;
}

int mm_map(struct task *task, phys_addr_t phys, uintptr_t virt)
{
    ASSERT(task->mm != NULL && phys != 0 && virt != 0);

    struct vm_struct *vm = &task->mm->vm_map;
    struct pg_struct *pg = &task->mm->pg_map;

    // 已分配但未映射 -> 已映射
    int set_status = ft_set_flags(&vm->table, virt, PG_SIZE, VM_MAP);
    if (set_status < 0)
    {
        return set_status;
    }

    struct list_node *node;
    node = list_traversal(&pg->list, traversal_by_phys, &phys);
    if (node == NULL)
    {
        PANIC("Cannot find the page struct of the physical address.");
    }

    struct page_struct *page_struct = NULL;
    page_struct = CONTAINER_OF(struct page_struct, node, node);

    // 这里其实和node != NULL等价
    ASSERT(page_struct->pfn == ADDR_TO_PFN(phys));
    page_struct->virt = virt;

    arch_mm_map(task, phys, virt);
    return 0;
}

int mm_unmap(struct task *task, uintptr_t virt)
{
    ASSERT(task->mm != NULL && virt != 0);

    struct vm_struct *vm = &task->mm->vm_map;
    struct pg_struct *pg = &task->mm->pg_map;

    // 已映射 -> 未分配
    int set_status = ft_set_flags(&vm->table, virt, PG_SIZE, VM_FRE);
    if (set_status < 0)
    {
        return set_status;
    }

    struct list_node *node;
    node = list_traversal(&pg->list, traversal_by_virt, &virt);
    if (node == NULL)
    {
        PANIC("Cannot find the page struct of the virtual address.");
    }

    struct page_struct *page_struct = NULL;
    page_struct = CONTAINER_OF(struct page_struct, node, node);

    // 这里其实和node != NULL等价
    ASSERT(page_struct->virt == virt);
    page_struct->virt = 0;

    arch_mm_unmap(task, virt);
    return 0;
}

// 将页映射为copy-on-write页
int mm_map_cow(struct task *task, phys_addr_t phys, uintptr_t virt)
{
    ASSERT(task->mm != NULL && phys != 0 && virt != 0);

    struct vm_struct *vm = &task->mm->vm_map;

    // 已映射的页转为写时复制
    if (ft_find(&vm->table, virt, VM_MAP))
    {
        int set_status = ft_set_flags(&vm->table, virt, PG_SIZE, VM_COW);
        if (set_status < 0)
        {
            return set_status;
        }
    }

    // 写时复制不对未映射的页生效
    ASSERT(!ft_find(&vm->table, virt, VM_UMP));

    // 加入写时复制表
    // 如果该页已是cow页但未复制,则不重复添加.
    if (!ft_find(&vm->table, virt, VM_COW))
    {
        int add_status = ft_add(&vm->table, virt, PG_SIZE, VM_COW);
        if (add_status < 0)
        {
            return add_status;
        }
    }

    // 设置页表中的标志
    arch_mm_map_cow(task, phys, virt);
    return 0;
}

void mm_free_address(uintptr_t addr, size_t pages)
{
    if (addr == 0)
    {
        return;
    }
    if (pages == 0 || pages > ((size_t)-1 >> PG_SIZE_SHIFT))
    {
        return;
    }
    size_t size = pages << PG_SIZE_SHIFT;
    if (!is_user_address(addr, size))
    {
        return;
    }

    struct task *task = get_current_task();
    if (task->mm == NULL)
    {
        return;
    }
    struct vm_struct *vm = &task->mm->vm_map;
    struct pg_struct *pg = &task->mm->pg_map;

    uintptr_t start = addr;

    struct page_struct *page_struct = NULL;
    size_t              remain_pages;
    for (remain_pages = pages; remain_pages > 0; remain_pages--)
    {
        page_struct = NULL;

        int mapped   = ft_find(&vm->table, start, VM_MAP);
        int unmapped = ft_find(&vm->table, start, VM_UMP);
        int cow      = ft_find(&vm->table, start, VM_COW);

        // 一张表里都没有: 已释放或未分配
        if (mapped + unmapped + cow == 0)
        {
            return;
        }
        // 同一页同时出现在多张表里: 内存管理状态已损坏
        if (mapped + unmapped + cow > 1)
        {
            PANIC("The address is in more than one vm table.");
        }

        if (unmapped)
        {
            // 已分配但未映射 -> 未分配
            int set_status = ft_set_flags(&vm->table, start, PG_SIZE, VM_FRE);
            if (set_status < 0)
            {
                return;
            }
        }
        else
        {
            page_struct = get_page_by_virt(pg, start);
            if (page_struct == NULL)
            {
                PANIC("Cannot find the page struct of the mapped address.");
            }
            if (cow)
            {
                // 写时复制 -> 已映射(mm_unmap 要求该页处于已映射状态)
                int set_status = 0;
                set_status = ft_set_flags(&vm->table, start, PG_SIZE, VM_MAP);
                if (set_status < 0)
                {
                    return;
                }
            }
            if (mm_unmap(task, page_struct->virt) < 0)
            {
                return;
            }
            flush_tlb(task, (void *)start);

            mm_free_a_page(PFN_TO_ADDR(page_struct->pfn));
        }

        start += PG_SIZE;
    }
    return;
}

phys_addr_t mm_allocate_a_page(void)
{
    struct task *task = get_current_task();
    ASSERT(task->mm != NULL);

    struct pg_struct *pg = &task->mm->pg_map;

    struct page_struct *page_struct = kmalloc(sizeof(*page_struct), 0, 0);
    if (page_struct == NULL)
    {
        return 0;
    }

    phys_addr_t addr = allocate_a_page();
    if (addr == 0)
    {
        kfree(page_struct);
        return 0;
    }
    page_struct->pfn  = ADDR_TO_PFN(addr);
    page_struct->virt = 0;
    list_append(&pg->list, &page_struct->node);
    return addr;
}

// 移除addr对应的page_struct,但不释放物理页
void mm_remove_a_page(phys_addr_t addr)
{
    size_t       pfn  = ADDR_TO_PFN(addr);
    struct task *task = get_current_task();
    ASSERT(task->mm != NULL);

    struct pg_struct *pg = &task->mm->pg_map;

    struct page_struct *page_struct = get_page_by_phys(pg, addr);
    if (page_struct == NULL)
    {
        PANIC("Cannot find the page struct of the physical page.");
    }
    ASSERT(page_struct->pfn == pfn);
    list_remove(&page_struct->node);
    kfree(page_struct);
    return;
}

// 在mm_remve_a_page的基础上释放对应的物理页
size_t mm_free_a_page(phys_addr_t addr)
{
    mm_remove_a_page(addr);
    return free_a_page(addr);
}
