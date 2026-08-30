// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/intr/handler.h>
#include <asm/ptrace.h>
#include <asm/utils/regs.h>

#include <mem.h>
#include <mem/page.h>
#include <std/string.h>
#include <sysinfo.h>
#include <task.h>
#include <task/schedule.h>

void set_pg_table(void *pg_table)
{
    set_cr3((uint64_t)pg_table);
    return;
}

static void page_map_sub(uint64_t *page_table, uintptr_t paddr, uintptr_t vaddr)
{
    paddr &= ~(PG_SIZE - 1);
    vaddr &= ~(PG_SIZE - 1);

    uint64_t *pg_dir, *pdpt, *pdt, *pt;
    uint64_t *pml4e, *pdpte, *pde, *pte;

    pg_dir = PHYS_TO_VIRT(page_table);
    pml4e  = pg_dir + GET_FIELD(vaddr, ADDR_PML4T_INDEX);
    if (!(*pml4e & PG_P))
    {
        pdpt = allocate_a_page();
        memset(pdpt, 0, PT_SIZE);
        *pml4e = (uintptr_t)VIRT_TO_PHYS(pdpt) | PG_DEFAULT_FLAGS;
    }
    pdpt  = PHYS_TO_VIRT(*pml4e & (~0xfff));
    pdpte = pdpt + GET_FIELD(vaddr, ADDR_PDPT_INDEX);
    if (!(*pdpte & PG_P))
    {
        pdt = allocate_a_page();
        memset(pdt, 0, PT_SIZE);
        *pdpte = (uintptr_t)VIRT_TO_PHYS(pdt) | PG_DEFAULT_FLAGS;
    }
    pdt = PHYS_TO_VIRT(*pdpte & (~0xfff));
    pde = pdt + GET_FIELD(vaddr, ADDR_PDT_INDEX);
    if (!(*pde & PG_P))
    {
        pt = allocate_a_page();
        memset(pt, 0, PT_SIZE);
        *pde = (uintptr_t)VIRT_TO_PHYS(pt) | PG_DEFAULT_FLAGS;
    }
    pt   = PHYS_TO_VIRT(*pde & (~0xfff));
    pte  = pt + GET_FIELD(vaddr, ADDR_PT_INDEX);
    *pte = paddr | PG_KERNEL_FLAGS;
    return;
}

void page_map(uint64_t *pg_dir, void *paddr, void *vaddr, uint64_t count)
{
    uintptr_t v, p;
    uint64_t  i;
    for (i = 0; i < count; i++)
    {
        v = (uintptr_t)vaddr + i * PG_SIZE;
        p = (uintptr_t)paddr + i * PG_SIZE;
        page_map_sub(pg_dir, p, v);
    }
    return;
}

void set_page_flags(uint64_t *pg_dir, void *vaddr, uint64_t flags)
{
    vaddr = (void *)((uintptr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t, *v_pml4e;
    uint64_t *pdpt, *v_pdpte, *pdpte;
    uint64_t *pdt, *v_pde, *pde;
    uint64_t *pt, *v_pte, *pte;

    v_pml4t = PHYS_TO_VIRT(pg_dir);
    v_pml4e = v_pml4t + GET_FIELD((uintptr_t)vaddr, ADDR_PML4T_INDEX);
    if (!(*v_pml4e & PG_P))
    {
        return;
    }

    pdpt    = (uint64_t *)(*v_pml4e & (~0xfff));
    pdpte   = pdpt + GET_FIELD((uintptr_t)vaddr, ADDR_PDPT_INDEX);
    v_pdpte = PHYS_TO_VIRT(pdpte);
    if (!(*v_pdpte & PG_P))
    {
        return;
    }

    pdt   = (uint64_t *)(*v_pdpte & (~0xfff));
    pde   = pdt + GET_FIELD((uintptr_t)vaddr, ADDR_PDT_INDEX);
    v_pde = PHYS_TO_VIRT(pde);
    if (!(*v_pde & PG_P))
    {
        return;
    }

    pt    = (uint64_t *)(*v_pde & (~0xfff));
    pte   = pt + GET_FIELD((uintptr_t)vaddr, ADDR_PT_INDEX);
    v_pte = PHYS_TO_VIRT(pte);
    if (!(*v_pte & PG_P))
    {
        return;
    }

    *v_pte = (*v_pte & ~0xfff) | flags;
    return;
}

void *to_physical_address(void *pg_dir, void *vaddr)
{
    uint64_t *v_pml4t, *v_pml4e;
    uint64_t *pdpt, *v_pdpte, *pdpte;
    uint64_t *pdt, *v_pde, *pde;
    uint64_t *pt, *v_pte, *pte;
    v_pml4t = PHYS_TO_VIRT(pg_dir);
    v_pml4e = v_pml4t + GET_FIELD((uintptr_t)vaddr, ADDR_PML4T_INDEX);
    if (!(*v_pml4e & PG_P))
    {
        return NULL;
    }
    pdpt    = (uint64_t *)(*v_pml4e & (~0xfff));
    pdpte   = pdpt + GET_FIELD((uintptr_t)vaddr, ADDR_PDPT_INDEX);
    v_pdpte = PHYS_TO_VIRT(pdpte);
    if (!(*v_pdpte & PG_P))
    {
        return NULL;
    }
    pdt   = (uint64_t *)(*v_pdpte & (~0xfff));
    pde   = pdt + GET_FIELD((uintptr_t)vaddr, ADDR_PDT_INDEX);
    v_pde = PHYS_TO_VIRT(pde);
    if (!(*v_pde & PG_P))
    {
        return NULL;
    }
    pt    = (uint64_t *)(*v_pde & (~0xfff));
    pte   = pt + GET_FIELD((uintptr_t)vaddr, ADDR_PT_INDEX);
    v_pte = PHYS_TO_VIRT(pte);
    if (!(*v_pte & PG_P))
    {
        return NULL;
    }
    return (void *)((*v_pte & ~0xfff) +
                    GET_FIELD((uintptr_t)vaddr, ADDR_OFFSET));
}

void arch_mm_map(struct task *task, void *phys, void *virt)
{
    page_map(task->pg_dir, phys, virt, 1);
    set_page_flags(task->pg_dir, virt, PG_USER_FLAGS);
    return;
}

void arch_mm_copy_on_write(struct task *task, void *phys, void *virt)
{
    page_map(task->pg_dir, phys, virt, 1);
    set_page_flags(task->pg_dir, virt, PG_USER_COW_FLAGS);
    return;
}

static void free_pt(uintptr_t pt)
{
    uint64_t *v_pt = PHYS_TO_VIRT(pt);

    free_a_page(v_pt);
    return;
}

static void free_pdt(uintptr_t pdt)
{
    uint64_t *v_pdt = PHYS_TO_VIRT(pdt);

    int i;
    for (i = 0; i < 512; i++)
    {
        if (v_pdt[i] & PG_P)
        {
            free_pt(v_pdt[i] & (~0xfff));
        }
    }
    free_a_page(v_pdt);
    return;
}

static void free_pdpt(uintptr_t pdpt)
{
    uint64_t *v_pdpt = PHYS_TO_VIRT(pdpt);

    int i;
    for (i = 0; i < 512; i++)
    {
        if (v_pdpt[i] & PG_P)
        {
            free_pdt(v_pdpt[i] & (~0xfff));
        }
    }
    free_a_page(v_pdpt);
    return;
}

void free_pg_table(uint64_t *pg_dir)
{
    if (pg_dir == NULL)
    {
        return;
    }
    uint64_t *v_pml4t = PHYS_TO_VIRT(pg_dir);

    int i;
    for (i = 0; i < 256; i++) // 仅限用户空间
    {
        if (v_pml4t[i] & PG_P)
        {
            free_pdpt(v_pml4t[i] & (~0xfff));
        }
    }
    free_a_page(v_pml4t);
    return;
}

void ASMLINKAGE arch_flush_tlb(void *addr);

static int page_cow_lock(struct task *task, void *addr, size_t cow_pfn)
{
    struct mm_struct *mm = task->mm;

    void *cow_page = (void *)PFN_TO_ADDR(cow_pfn);

    // 只有一个进程引用: 不复制
    if (page_reference_read_lock(cow_pfn) == 1)
    {
        free_table_remove(&mm->vm_map.copy_on_write, (uintptr_t)addr, PG_SIZE);
        free_table_add(&mm->vm_map.mapped, (uintptr_t)addr, PG_SIZE);
        set_page_flags(task->pg_dir, addr, PG_USER_FLAGS);
        arch_flush_tlb(addr);
        return 0;
    }

    // 多个进程引用: 复制新页
    void *new_page = mm_allocate_a_page_lock();
    if (new_page == NULL)
    {
        return -1;
    }
    memcpy(PHYS_TO_VIRT(new_page), PHYS_TO_VIRT(cow_page), PG_SIZE);
    free_table_remove(&mm->vm_map.copy_on_write, (uintptr_t)addr, PG_SIZE);
    mm_map(task, new_page, addr);

    // 通过free解除对旧页的引用
    if (mm_free_a_page_lock(cow_page, cow_pfn) == 0)
    {
        return -1;
    }
    arch_flush_tlb(addr);
    return 0;
}

void page_faule(struct pt_regs *regs)
{
    struct task      *task = get_current_task();
    struct mm_struct *mm   = task->mm;

    // 内核态缺页
    if (mm == NULL)
    {
        general_handler(regs);
    }

    uintptr_t fault_address = get_cr2() & ~(PG_SIZE - 1);

    int unmapped = free_table_find(&mm->vm_map.unmapped, fault_address);
    int cow      = free_table_find(&mm->vm_map.copy_on_write, fault_address);
    // 访问非法地址触发异常
    if (!unmapped && !cow)
    {
        general_handler(regs);
    }

    if (unmapped)
    {
        // 已分配地址,未分配页
        void *phy_page = mm_allocate_a_page();
        if (phy_page == NULL)
        {
            general_handler(regs);
        }
        mm_map(task, phy_page, (void *)fault_address);
        arch_flush_tlb((void *)fault_address);
        return;
    }

    // copy-on-write
    if (!(regs->error_code & PG_RW_W))
    {
        general_handler(regs);
    }

    void *cow_page = to_physical_address(task->pg_dir, (void *)fault_address);
    if (cow_page == NULL)
    {
        general_handler(regs);
    }
    size_t cow_pfn = ADDR_TO_PFN((uintptr_t)cow_page);

    page_mgr_lock();
    page_struct_lock(cow_pfn);
    int ret = page_cow_lock(task, (void *)fault_address, cow_pfn);
    page_struct_unlock(cow_pfn);
    page_mgr_unlock();
    if (ret < 0)
    {
        general_handler(regs);
    }
    return;
}