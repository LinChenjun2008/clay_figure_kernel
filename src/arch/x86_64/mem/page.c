// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/mem/page.h>
#include <asm/sync/spinlock.h>
#include <asm/utils.h>

#include <efi.h>
#include <mm_struct.h>
#include <print.h>
#include <std/string.h>

static struct mm_block page_mm_blocks[2048];

struct page_man
{
    struct spinlock lock;
    struct mm       mm_struct;
};

static struct page_man page_man;

static enum mm_type get_page_type(enum efi_memory_type efi_type)
{
    switch (efi_type)
    {
        case EFI_CONVENTIONAL_MEMORY:
        case EFI_BOOT_SERVICES_CODE:
        case EFI_BOOT_SERVICES_DATA:
        case EFI_LOADER_CODE:
            return MM_TYPE_FREE;

        case EFI_LOADER_DATA:
        case EFI_RUNTIME_SERVICES_CODE:
        case EFI_RUNTIME_SERVICES_DATA:
        case EFI_MEMORY_MAPPED_IO:
        case EFI_MEMORY_MAPPED_IO_PORT_SPACE:
        case EFI_PAL_CODE:
        case EFI_RESERVED_TYPE:
        case EFI_ACPI_RECLAIM_MEMORY:
        case EFI_ACPI_MEMORY_NVS:
            return MM_TYPE_RESERVED;

        case EFI_UNUSABLE_MEMORY:
        case EFI_MAX_MEMORY_TYPE:
            return MAX_MM_TYPE;
    }
    return MAX_MM_TYPE;
}

void page_init(struct boot_info *boot_info)
{
    init_mm_struct(&page_man.mm_struct, page_mm_blocks, 2048);
    init_spinlock(&page_man.lock);

    struct efi_memory_descriptor *memmap;
    memmap = (struct efi_memory_descriptor *)boot_info->memory_map.buffer;

    size_t map_size   = boot_info->memory_map.map_size;
    size_t desc_size  = boot_info->memory_map.descriptor_size;
    int    desc_count = map_size / desc_size;

    uintptr_t curr_start = 0;
    uintptr_t curr_end   = 0;
    size_t    curr_size  = 0;
    uint64_t  curr_pages = 0;

    enum mm_type curr_type = MAX_MM_TYPE;

    int i;
    for (i = 0; i < desc_count; i++)
    {
        curr_type = get_page_type(memmap[i].type);

        if (curr_type != MM_TYPE_FREE)
        {
            continue;
        }

        curr_start = memmap[i].physical_start;
        curr_pages = memmap[i].number_of_pages;
        curr_size  = (curr_pages << 12);
        curr_end   = curr_start + curr_size;

        if (curr_end < 0x100000)
        {
            continue;
        }

        if (curr_start < 0x100000)
        {
            curr_start = 0x100000;
            curr_size  = curr_end - curr_start;
            curr_pages = curr_size >> 12;
        }
        curr_start = (uintptr_t)PHYS_TO_VIRT(curr_start);
        mm_add(&page_man.mm_struct, curr_start, curr_size);
    }
    return;
}

int allocate_pages(size_t pages, void **addr)
{
    uintptr_t page_addr;

    spin_lock(&page_man.lock);
    page_addr = mm_allocate(&page_man.mm_struct, pages * PG_SIZE);
    spin_unlock(&page_man.lock);

    if (page_addr == -1UL)
    {
        *addr = NULL;
        return -1;
    }
    ASSERT(addr != NULL);
    *addr = (void *)page_addr;
    return 0;
}

void free_pages(void **addr, size_t pages)
{
    ASSERT(addr != NULL);
    uintptr_t page_addr = (uintptr_t)*addr;
    ASSERT(page_addr != 0 && !(page_addr & (PG_SIZE - 1)));

    int ret = 0;
    spin_lock(&page_man.lock);
    ret = mm_add(&page_man.mm_struct, page_addr, pages * PG_SIZE);
    spin_unlock(&page_man.lock);
    if (ret != 0)
    {
        printk(MSG_ERR "free_pages: failed: %p.\n", page_addr);
    }
    *addr = NULL;
    return;
}

uint64_t *get_page_table(void)
{
    return (uint64_t *)get_cr3();
}

void set_page_table(void *page_table)
{
    set_cr3((uint64_t)page_table);
    return;
}

static void page_map_sub(uint64_t *page_table, uintptr_t paddr, uintptr_t vaddr)
{
    paddr &= ~(PG_SIZE - 1);
    vaddr &= ~(PG_SIZE - 1);

    uint64_t *pml4t, *pdpt, *pdt, *pt;
    uint64_t *pml4e, *pdpte, *pde, *pte;

    pml4t = PHYS_TO_VIRT(page_table);
    pml4e = pml4t + GET_FIELD(vaddr, ADDR_PML4T_INDEX);
    if (!(*pml4e & PG_P))
    {
        allocate_pages(1, (void **)&pdpt);
        memset(pdpt, 0, PT_SIZE);
        *pml4e = (uintptr_t)VIRT_TO_PHYS(pdpt) | PG_DEFAULT_FLAGS;
    }
    pdpt  = PHYS_TO_VIRT(*pml4e & (~0xfff));
    pdpte = pdpt + GET_FIELD(vaddr, ADDR_PDPT_INDEX);
    if (!(*pdpte & PG_P))
    {
        allocate_pages(1, (void **)&pdt);
        memset(pdt, 0, PT_SIZE);
        *pdpte = (uintptr_t)VIRT_TO_PHYS(pdt) | PG_DEFAULT_FLAGS;
    }
    pdt = PHYS_TO_VIRT(*pdpte & (~0xfff));
    pde = pdt + GET_FIELD(vaddr, ADDR_PDT_INDEX);
    if (!(*pde & PG_P))
    {
        allocate_pages(1, (void **)&pt);
        memset(pt, 0, PT_SIZE);
        *pde = (uintptr_t)VIRT_TO_PHYS(pt) | PG_DEFAULT_FLAGS;
    }
    pt   = PHYS_TO_VIRT(*pde & (~0xfff));
    pte  = pt + GET_FIELD(vaddr, ADDR_PT_INDEX);
    *pte = paddr | PG_DEFAULT_FLAGS;
    return;
}

void page_map(uint64_t *pml4t, void *paddr, void *vaddr, uint64_t count)
{
    uintptr_t v, p;
    uint64_t  i;
    for (i = 0; i < count; i++)
    {
        v = (uintptr_t)vaddr + i * PG_SIZE;
        p = (uintptr_t)paddr + i * PG_SIZE;
        page_map_sub(pml4t, p, v);
    }
    return;
}

void *to_physical_address(void *pml4t, void *vaddr)
{
    uint64_t *v_pml4t, *v_pml4e;
    uint64_t *pdpt, *v_pdpte, *pdpte;
    uint64_t *pdt, *v_pde, *pde;
    uint64_t *pt, *v_pte, *pte;
    v_pml4t = PHYS_TO_VIRT(pml4t);
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
