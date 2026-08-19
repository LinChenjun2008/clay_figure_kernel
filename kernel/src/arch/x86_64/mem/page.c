// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>
#include <asm/utils.h>

#include <std/string.h>

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
        pdpt = allocate_pages(1);
        memset(pdpt, 0, PT_SIZE);
        *pml4e = (uintptr_t)VIRT_TO_PHYS(pdpt) | PG_DEFAULT_FLAGS;
    }
    pdpt  = PHYS_TO_VIRT(*pml4e & (~0xfff));
    pdpte = pdpt + GET_FIELD(vaddr, ADDR_PDPT_INDEX);
    if (!(*pdpte & PG_P))
    {
        pdt = allocate_pages(1);
        memset(pdt, 0, PT_SIZE);
        *pdpte = (uintptr_t)VIRT_TO_PHYS(pdt) | PG_DEFAULT_FLAGS;
    }
    pdt = PHYS_TO_VIRT(*pdpte & (~0xfff));
    pde = pdt + GET_FIELD(vaddr, ADDR_PDT_INDEX);
    if (!(*pde & PG_P))
    {
        pt = allocate_pages(1);
        memset(pt, 0, PT_SIZE);
        *pde = (uintptr_t)VIRT_TO_PHYS(pt) | PG_DEFAULT_FLAGS;
    }
    pt   = PHYS_TO_VIRT(*pde & (~0xfff));
    pte  = pt + GET_FIELD(vaddr, ADDR_PT_INDEX);
    *pte = paddr | PG_DEFAULT_FLAGS;
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

static void free_pt(uintptr_t pt)
{
    uint64_t *v_pt = PHYS_TO_VIRT(pt);

    free_pages(v_pt, 1);
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
    free_pages(v_pdt, 1);
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
    free_pages(v_pdpt, 1);
    return;
}

void free_pg_table(uint64_t *pg_dir)
{
    uint64_t *v_pml4t = PHYS_TO_VIRT(pg_dir);

    int i;
    for (i = 0; i < 256; i++) // 仅限用户空间
    {
        if (v_pml4t[i] & PG_P)
        {
            free_pdpt(v_pml4t[i] & (~0xfff));
        }
    }
    free_pages(v_pml4t, 1);
    return;
}