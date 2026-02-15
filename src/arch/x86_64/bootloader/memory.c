// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include "bootloader.h"

efi_status_t get_memory_map(memory_map_t *memmap)
{
    efi_status_t status = EFI_SUCCESS;

    status = boot_services->allocate_pool(
        EFI_LOADER_DATA, memmap->map_size, &memmap->buffer
    );
    if (EFI_ERROR(status))
    {
        printf(
            L"get_memory_map: boot_services->allocate_pool: ERROR(%d).\n\r",
            status
        );
        return status;
    }
    status = boot_services->get_memory_map(
        &memmap->map_size,
        (efi_memory_descriptor_t *)memmap->buffer,
        &memmap->map_key,
        &memmap->descriptor_size,
        &memmap->descriptor_version
    );
    return status;
}

static void page_map_sub(uint64_t *pml4t, void *paddr, void *vaddr)
{
    paddr = (void *)((uintptr_t)paddr & ~(PG_SIZE - 1));
    vaddr = (void *)((uintptr_t)vaddr & ~(PG_SIZE - 1));

    uint64_t *pml4e;
    uint64_t *pdpt = NULL, *pdpte;
    uint64_t *pdt  = NULL, *pde;
    uint64_t *pt   = NULL, *pte;

    pml4e = pml4t + GET_FIELD((uintptr_t)vaddr, ADDR_PML4T_INDEX);

    efi_status_t status;
    if (!(*pml4e & PG_P))
    {
        status = boot_services->allocate_pages(
            EFI_ALLOCATE_ANY_PAGES,
            EFI_LOADER_DATA,
            1,
            (efi_physical_address_t *)&pdpt
        );
        if (EFI_ERROR(status))
        {
            printf(
                L"page_map_sub: boot_services->allocate_pages(pdpt): "
                L"ERROR(%d).\n\r",
                status
            );
            return;
        }
        boot_services->set_mem(pdpt, PT_SIZE, 0);
        *pml4e = (uintptr_t)pdpt | PG_US_U | PG_RW_W | PG_P;
    }

    pdpt  = (uint64_t *)(*pml4e & (~0xfff));
    pdpte = pdpt + GET_FIELD((uintptr_t)vaddr, ADDR_PDPT_INDEX);
    if (!(*pdpte & PG_P))
    {
        status = boot_services->allocate_pages(
            EFI_ALLOCATE_ANY_PAGES,
            EFI_LOADER_DATA,
            1,
            (efi_physical_address_t *)&pdt
        );
        if (EFI_ERROR(status))
        {
            printf(
                L"page_map_sub: boot_services->allocate_pages(pdt): "
                L"ERROR(%d).\n\r",
                status
            );
            return;
        }
        boot_services->set_mem(pdt, PT_SIZE, 0);
        *pdpte = (uintptr_t)pdt | PG_US_U | PG_RW_W | PG_P;
    }

    pdt = (uint64_t *)(*pdpte & (~0xfff));
    pde = pdt + GET_FIELD((uintptr_t)vaddr, ADDR_PDT_INDEX);
    if (!(*pde & PG_P))
    {
        status = boot_services->allocate_pages(
            EFI_ALLOCATE_ANY_PAGES,
            EFI_LOADER_DATA,
            1,
            (efi_physical_address_t *)&pt
        );
        if (EFI_ERROR(status))
        {
            printf(
                L"page_map_sub: boot_services->allocate_pages(pt): "
                L"ERROR(%d).\n\r",
                status
            );
            return;
        }
        boot_services->set_mem(pt, PT_SIZE, 0);
        *pde = (uintptr_t)pt | PG_US_U | PG_RW_W | PG_P;
    }

    pt   = (uint64_t *)(*pde & (~0xfff));
    pte  = pt + GET_FIELD((uintptr_t)vaddr, ADDR_PT_INDEX);
    *pte = (uintptr_t)paddr | PG_DEFAULT_FLAGS;
    return;
}

static void
boot_page_map(uint64_t *pml4t, void *paddr, void *vaddr, uint64_t pages)
{
    uint64_t i;
    for (i = 0; i < pages; i++)
    {
        page_map_sub(
            pml4t,
            (void *)((uintptr_t)paddr + i * PG_SIZE),
            (void *)((uintptr_t)vaddr + i * PG_SIZE)
        );
    }
}

efi_status_t create_page_table(void *pml4t)
{
    efi_status_t status         = EFI_SUCCESS;
    uint64_t    *page_table_pos = NULL;

    status = boot_services->allocate_pages(
        EFI_ALLOCATE_ANY_PAGES,
        EFI_LOADER_DATA,
        1,
        (efi_physical_address_t *)&page_table_pos
    );
    if (EFI_ERROR(status))
    {
        printf(
            L"create_page_table: boot_services->allcate_pages: ERROR(%d).\n\r",
            status
        );
        return status;
    }

    boot_services->set_mem(page_table_pos, PT_SIZE, 0);

    uintptr_t *paddr, *vaddr;

    // 0 - 4 GiB
    paddr = (uintptr_t *)0;
    vaddr = PHYS_TO_VIRT(paddr);
    boot_page_map(page_table_pos, paddr, vaddr, 1 << 20);
    boot_page_map(page_table_pos, paddr, paddr, 1 << 20);

    // frame buffer
    paddr          = (void *)gop->mode->frame_buffer_base;
    vaddr          = PHYS_TO_VIRT(paddr);
    uint64_t pages = (gop->mode->frame_buffer_size + PG_SIZE - 1) / PG_SIZE;
    boot_page_map(page_table_pos, paddr, vaddr, pages);

    // kernel code
    boot_page_map(page_table_pos, (void *)0, (void *)KERNEL_TEXT_BASE, 512);

    *(uint64_t **)pml4t = page_table_pos;
    return status;
}