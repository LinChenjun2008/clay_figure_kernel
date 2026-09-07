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

void set_pg_table(phys_addr_t pg_table)
{
    set_cr3(pg_table);
    return;
}

static void page_map_sub(phys_addr_t pg_dir, phys_addr_t phys, uintptr_t virt)
{
    phys &= ~(phys_addr_t)(PG_SIZE - 1);
    virt &= ~(uintptr_t)(PG_SIZE - 1);

    uint64_t *pml4t, *pdpt, *pdt, *pt;
    uint64_t *pml4e, *pdpte, *pde, *pte;

    pml4t = PHYS_TO_VIRT(pg_dir);
    pml4e = pml4t + GET_FIELD(virt, ADDR_PML4T_INDEX);
    if (!(*pml4e & PG_P))
    {
        pdpt = allocate_a_page();
        memset(pdpt, 0, PT_SIZE);
        *pml4e = VIRT_TO_PHYS(pdpt) | PG_DEFAULT_FLAGS;
    }
    pdpt  = PHYS_TO_VIRT(*pml4e & (~0xfffUL));
    pdpte = pdpt + GET_FIELD(virt, ADDR_PDPT_INDEX);
    if (!(*pdpte & PG_P))
    {
        pdt = allocate_a_page();
        memset(pdt, 0, PT_SIZE);
        *pdpte = VIRT_TO_PHYS(pdt) | PG_DEFAULT_FLAGS;
    }
    pdt = PHYS_TO_VIRT(*pdpte & (~0xfffUL));
    pde = pdt + GET_FIELD(virt, ADDR_PDT_INDEX);
    if (!(*pde & PG_P))
    {
        pt = allocate_a_page();
        memset(pt, 0, PT_SIZE);
        *pde = VIRT_TO_PHYS(pt) | PG_DEFAULT_FLAGS;
    }
    pt   = PHYS_TO_VIRT(*pde & (~0xfffUL));
    pte  = pt + GET_FIELD(virt, ADDR_PT_INDEX);
    *pte = phys | PG_KERNEL_FLAGS;
    return;
}

void page_map(phys_addr_t pg_dir, phys_addr_t phys, uintptr_t virt, int count)
{
    off_t offset = 0;
    int   i;
    for (i = 0; i < count; i++)
    {
        offset = i * PG_SIZE;
        page_map_sub(pg_dir, phys + offset, virt + offset);
    }
    return;
}

void set_page_flags(phys_addr_t pg_dir, uintptr_t virt, uint64_t flags)
{
    virt &= ~(uintptr_t)(PG_SIZE - 1);

    uint64_t *pml4t, *pdpt, *pdt, *pt;
    uint64_t *pml4e, *pdpte, *pde, *pte;

    pml4t = PHYS_TO_VIRT(pg_dir);
    pml4e = pml4t + GET_FIELD(virt, ADDR_PML4T_INDEX);
    if (!(*pml4e & PG_P))
    {
        return;
    }

    pdpt  = PHYS_TO_VIRT(*pml4e & (~0xfffUL));
    pdpte = pdpt + GET_FIELD(virt, ADDR_PDPT_INDEX);
    if (!(*pdpte & PG_P))
    {
        return;
    }

    pdt = PHYS_TO_VIRT(*pdpte & (~0xfffUL));
    pde = pdt + GET_FIELD(virt, ADDR_PDT_INDEX);
    if (!(*pde & PG_P))
    {
        return;
    }

    pt  = PHYS_TO_VIRT(*pde & (~0xfffUL));
    pte = pt + GET_FIELD(virt, ADDR_PT_INDEX);
    if (!(*pte & PG_P))
    {
        return;
    }

    *pte = (*pte & ~0xfffUL) | flags;
    return;
}

phys_addr_t to_physical_address(phys_addr_t pg_dir, uintptr_t virt)
{
    uint64_t *pml4t, *pdpt, *pdt, *pt;
    uint64_t *pml4e, *pdpte, *pde, *pte;

    pml4t = PHYS_TO_VIRT(pg_dir);
    pml4e = pml4t + GET_FIELD(virt, ADDR_PML4T_INDEX);
    if (!(*pml4e & PG_P))
    {
        return 0;
    }
    pdpt  = PHYS_TO_VIRT(*pml4e & (~0xfffUL));
    pdpte = pdpt + GET_FIELD(virt, ADDR_PDPT_INDEX);
    if (!(*pdpte & PG_P))
    {
        return 0;
    }
    pdt = PHYS_TO_VIRT(*pdpte & (~0xfffUL));
    pde = pdt + GET_FIELD(virt, ADDR_PDT_INDEX);
    if (!(*pde & PG_P))
    {
        return 0;
    }
    pt  = PHYS_TO_VIRT(*pde & (~0xfffUL));
    pte = pt + GET_FIELD(virt, ADDR_PT_INDEX);
    if (!(*pte & PG_P))
    {
        return 0;
    }
    return (phys_addr_t)((*pte & ~0xfffUL) + GET_FIELD(virt, ADDR_OFFSET));
}

void arch_mm_map(struct task *task, phys_addr_t phys, uintptr_t virt)
{
    page_map(task->pg_dir, phys, virt, 1);
    set_page_flags(task->pg_dir, virt, PG_USER_FLAGS);
    return;
}

void arch_mm_unmap(struct task *task, uintptr_t virt)
{
    set_page_flags(task->pg_dir, virt, PG_UNMAPPED);
    return;
}

void arch_mm_map_cow(struct task *task, phys_addr_t phys, uintptr_t virt)
{
    page_map(task->pg_dir, phys, virt, 1);
    set_page_flags(task->pg_dir, virt, PG_USER_COW_FLAGS);
    return;
}

static void free_pt(phys_addr_t pt)
{
    free_a_page(PHYS_TO_VIRT(pt));
    return;
}

static void free_pdt(phys_addr_t pdt)
{
    uint64_t *v_pdt = PHYS_TO_VIRT(pdt);

    int i;
    for (i = 0; i < 512; i++)
    {
        if (v_pdt[i] & PG_P)
        {
            free_pt(v_pdt[i] & (~0xfffUL));
        }
    }
    free_a_page(v_pdt);
    return;
}

static void free_pdpt(phys_addr_t pdpt)
{
    uint64_t *v_pdpt = PHYS_TO_VIRT(pdpt);

    int i;
    for (i = 0; i < 512; i++)
    {
        if (v_pdpt[i] & PG_P)
        {
            free_pdt(v_pdpt[i] & (~0xfffUL));
        }
    }
    free_a_page(v_pdpt);
    return;
}

void free_pg_table(phys_addr_t pg_dir)
{
    if (pg_dir == 0)
    {
        return;
    }
    uint64_t *v_pml4t = PHYS_TO_VIRT(pg_dir);

    int i;
    for (i = 0; i < 256; i++) // 仅限用户空间
    {
        if (v_pml4t[i] & PG_P)
        {
            free_pdpt(v_pml4t[i] & (~0xfffUL));
        }
    }
    free_a_page(v_pml4t);
    return;
}

void ASMLINKAGE arch_flush_tlb(void *addr);

static int page_lazy_allocate(struct task *task, uintptr_t fault_address)
{
    phys_addr_t phy_page = mm_allocate_a_page();
    if (phy_page == 0)
    {
        return -1;
    }
    mm_map(task, phy_page, fault_address);
    arch_flush_tlb((void *)fault_address);
    return 0;
}

static int page_copy_on_write(struct task *task, uintptr_t fault_address)
{
    struct mm_struct *mm = task->mm;

    phys_addr_t cow_page = to_physical_address(task->pg_dir, fault_address);
    // cow页已经映射,不可能为NULL
    if (cow_page == 0)
    {
        return -1;
    }
    size_t cow_pfn = ADDR_TO_PFN(cow_page);

    int ret = 0;


    // 进入cow页临界区,对cow->lock加锁
    // 临界区内执行写时复制
    page_struct_lock(cow_pfn);
    uint32_t ref_count = page_reference_read_lock(cow_pfn);

    if (ref_count == 1)
    {
        free_table_remove(&mm->vm_map.copy_on_write, fault_address, PG_SIZE);
        free_table_add(&mm->vm_map.mapped, fault_address, PG_SIZE);
        set_page_flags(task->pg_dir, fault_address, PG_USER_FLAGS);
        arch_flush_tlb((void *)fault_address);
    }
    else
    {
        phys_addr_t new_page = mm_allocate_a_page();
        if (new_page == 0)
        {
            ret = -1;
            goto fail;
        }
        memcpy(PHYS_TO_VIRT(new_page), PHYS_TO_VIRT(cow_page), PG_SIZE);
        // 从cow中移除,转入unmapped表,由mm_map重新映射
        free_table_remove(&mm->vm_map.copy_on_write, fault_address, PG_SIZE);
        free_table_add(&mm->vm_map.unmapped, fault_address, PG_SIZE);
        mm_map(task, new_page, fault_address);

        // 减少引用,并从pg_struct链表中移除
        // 因为此时cow_page已经不属于当前任务了.
        page_reference_dec_lock(cow_pfn);
        mm_remove_a_page(cow_page);
        arch_flush_tlb((void *)fault_address);
    }
fail:
    page_struct_unlock(cow_pfn);
    return ret;
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
        if (page_lazy_allocate(task, fault_address) < 0)
        {
            general_handler(regs);
        }
        return;
    }
    if (cow)
    {
        if (!(regs->error_code & PG_RW_W))
        {
            general_handler(regs);
        }
        if (page_copy_on_write(task, fault_address) < 0)
        {
            general_handler(regs);
        }
        return;
    }

    general_handler(regs);

    return;
}