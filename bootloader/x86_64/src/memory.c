// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <bootloader.h>

void *efi_malloc(size_t size)
{
    efi_status_t status = EFI_SUCCESS;

    void *ret = NULL;
    status = boot_services->allocate_pool(EFI_LOADER_DATA, size, (void **)&ret);
    if (EFI_ERROR(status))
    {
        printf(L"efi_malloc: allocate failed: %d.\n", size);
        return NULL;
    }
    return ret;
}

efi_status_t get_memory_map(struct memory_map *memmap)
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
        (struct efi_memory_descriptor *)memmap->buffer,
        &memmap->map_key,
        &memmap->descriptor_size,
        &memmap->descriptor_version
    );
    return status;
}

static void page_map_sub(uint64_t *pg_dir, void *paddr, void *vaddr)
{
    paddr = (void *)((uintptr_t)paddr & ~(PG_SIZE - 1));
    vaddr = (void *)((uintptr_t)vaddr & ~(PG_SIZE - 1));

    uint64_t *pml4e;
    uint64_t *pdpt = NULL, *pdpte;
    uint64_t *pdt  = NULL, *pde;
    uint64_t *pt   = NULL, *pte;

    pml4e = pg_dir + GET_FIELD((uintptr_t)vaddr, ADDR_PML4T_INDEX);

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
boot_page_map(uint64_t *pg_dir, void *paddr, void *vaddr, uint64_t pages)
{
    uint64_t i;
    for (i = 0; i < pages; i++)
    {
        page_map_sub(
            pg_dir,
            (void *)((uintptr_t)paddr + i * PG_SIZE),
            (void *)((uintptr_t)vaddr + i * PG_SIZE)
        );
    }
}

efi_status_t create_page_table(void *pg_dir)
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
    printf(L"mmap: %p - %p.\r\n", paddr, vaddr);
    boot_page_map(page_table_pos, paddr, vaddr, 1 << 20);
    boot_page_map(page_table_pos, paddr, paddr, 1 << 20);

    // frame buffer
    paddr          = (void *)gop->mode->frame_buffer_base;
    vaddr          = PHYS_TO_VIRT(paddr);
    uint64_t pages = (gop->mode->frame_buffer_size + PG_SIZE - 1) / PG_SIZE;
    printf(L"mmap: %p - %p.\r\n", paddr, vaddr);
    boot_page_map(page_table_pos, paddr, vaddr, pages);

    // kernel code
    printf(L"mmap: %p - %p.\r\n", 0, KERNEL_TEXT_BASE);
    boot_page_map(page_table_pos, (void *)0, (void *)KERNEL_TEXT_BASE, 512);

    *(uint64_t **)pg_dir = page_table_pos;
    return status;
}

// 计算efi_memory_descriptor的个数
static int efi_mem_desc_count(struct memory_map *memmap)
{
    size_t map_size  = memmap->map_size;
    size_t desc_size = memmap->descriptor_size;
    return map_size / desc_size;
}

// 读取efi_memory_descriptor[i]
static struct efi_memory_descriptor *
read_efi_mem_desc(struct memory_map *memmap, int i)
{
    void *ret;
    ret = ((char *)memmap->buffer + memmap->descriptor_size * i);
    return (struct efi_memory_descriptor *)ret;
}

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

// 计算可用内存的最大pfn
static size_t calculate_max_pfn(struct memory_map *memmap)
{
    enum mm_type curr_type = MAX_MM_TYPE;

    uintptr_t curr_start = 0;
    uintptr_t curr_end   = 0;
    size_t    curr_size  = 0;
    uint64_t  curr_pages = 0;

    struct efi_memory_descriptor *mem_desc = NULL;

    int desc_count = efi_mem_desc_count(memmap);

    size_t max_pfn = 0;
    int    i;

    for (i = 0; i < desc_count; i++)
    {
        mem_desc  = read_efi_mem_desc(memmap, i);
        curr_type = get_page_type(mem_desc->type);

        curr_start = mem_desc->physical_start;
        curr_pages = mem_desc->number_of_pages;
        curr_size  = (curr_pages << 12);
        curr_end   = curr_start + curr_size;

        if (curr_type != MM_TYPE_FREE)
        {
            continue;
        }
        size_t end_pfn = (curr_end) >> PAGE_SIZE_SHIFT;
        if (end_pfn > max_pfn) max_pfn = end_pfn;
    }
    return max_pfn;
}

// 为内核初始化page_mgr
efi_status_t init_page_mgr(struct system_info *system_info)
{
    struct memory_map *memmap = &system_info->boot_info->memory_map;

    size_t max_pfn  = calculate_max_pfn(memmap);
    size_t map_size = (max_pfn >> 3) + 1;
    void  *map      = efi_malloc(map_size);

    size_t pages_size = sizeof(system_info->page_mgr->pages[0]) * (max_pfn + 1);
    void  *pages      = efi_malloc(pages_size);

    system_info->page_mgr->bitmap.map_size = map_size;
    system_info->page_mgr->bitmap.map      = PHYS_TO_VIRT(map);

    system_info->page_mgr->pages   = PHYS_TO_VIRT(pages);
    system_info->page_mgr->max_pfn = max_pfn;
    return EFI_SUCCESS;
}