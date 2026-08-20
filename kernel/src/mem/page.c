// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>
#include <asm/sync/spinlock.h>

#include <efi.h>
#include <mem/struct.h>
#include <print.h>
#include <sync/atomic.h>
#include <sysinfo.h>

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

// 计算efi_memory_descriptor的个数
static int efi_mem_desc_count(struct memory_map *memmap)
{
    size_t map_size  = memmap->map_size;
    size_t desc_size = memmap->descriptor_size;
    return map_size / desc_size;
}

static struct efi_memory_descriptor *
read_efi_mem_desc(struct memory_map *memmap, int i)
{
    void *ret;
    ret = ((char *)memmap->buffer + memmap->descriptor_size * i);
    return (struct efi_memory_descriptor *)ret;
}

static void
find_free_pages(struct page_mgr *page_mgr, struct memory_map *memmap)
{
    enum mm_type curr_type = MAX_MM_TYPE;

    uintptr_t curr_start = 0;
    uint64_t  curr_pages = 0;
    size_t    curr_pfn   = 0;

    struct efi_memory_descriptor *mem_desc = NULL;

    int desc_count = efi_mem_desc_count(memmap);

    int i;

    for (i = 0; i < desc_count; i++)
    {
        mem_desc  = read_efi_mem_desc(memmap, i);
        curr_type = get_page_type(mem_desc->type);

        curr_start = mem_desc->physical_start;
        curr_pages = mem_desc->number_of_pages;
        curr_pfn   = curr_start >> PAGE_SIZE_SHIFT;
        if (curr_type != MM_TYPE_FREE)
        {
            continue;
        }
        bitmap_set(&page_mgr->bitmap, curr_pfn, 1, curr_pages);
    }
    // 1MiB
    bitmap_set(&page_mgr->bitmap, 0, 0, 256);
    return;
}

void page_mgr_init(struct system_info *system_info)
{
    struct page_mgr   *page_mgr = system_info->page_mgr;
    struct memory_map *memmap   = &system_info->boot_info->memory_map;

    init_spinlock(&page_mgr->lock);
    struct bitmap *bitmap = &page_mgr->bitmap;
    init_bitmap(bitmap, bitmap->map_size, bitmap->map);
    find_free_pages(system_info->page_mgr, memmap);

    size_t i;
    for (i = 0; i <= page_mgr->max_pfn; i++)
    {
        atomic_set(&page_mgr->pages[i].reference_count, 0);
        page_mgr->pages[i].flags = 0;
        page_mgr->pages[i].count = 0;
    }
    return;
}

static void page_reference_inc_lock(size_t pfn)
{
    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;
    if (pfn > page_mgr->max_pfn)
    {
        return;
    }
    atomic_inc(&page_mgr->pages[pfn].reference_count);
    return;
}

void page_reference_inc(size_t pfn)
{
    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    spin_lock(&page_mgr->lock);
    page_reference_inc_lock(pfn);
    spin_unlock(&page_mgr->lock);
    return;
}

static uint64_t page_reference_dec_lock(size_t pfn)
{

    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;
    if (pfn > page_mgr->max_pfn)
    {
        return -1;
    }
    return atomic_dec(&page_mgr->pages[pfn].reference_count);
}

uint64_t page_reference_dec(size_t pfn)
{
    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    spin_lock(&page_mgr->lock);
    uint64_t ret = page_reference_dec_lock(pfn);
    spin_unlock(&page_mgr->lock);
    return ret;
}

static void *allocate_pages_lock(size_t pages)
{
    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;
    size_t              pfn         = bitmap_find(&page_mgr->bitmap, 1, pages);
    if (pfn == -1UL)
    {
        return NULL;
    }
    bitmap_set(&page_mgr->bitmap, pfn, 0, pages);
    page_reference_inc_lock(pfn);
    page_mgr->pages[pfn].flags = PAGE_HEAD;
    page_mgr->pages[pfn].count = pages;

    uintptr_t ret = pfn << PAGE_SIZE_SHIFT;
    return PHYS_TO_VIRT(ret);
}

void *allocate_pages(size_t pages)
{
    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    spin_lock(&page_mgr->lock);
    void *ret = allocate_pages_lock(pages);
    spin_unlock(&page_mgr->lock);

    return ret;
}

static void free_pages_lock(void *addr, size_t pages)
{
    if (addr == NULL)
    {
        printk(MSG_WARN "free_pages: Free null point.\n");
        return;
    }
    ASSERT(((uintptr_t)addr & (PG_SIZE - 1)) == 0);

    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    size_t pfn = (uintptr_t)VIRT_TO_PHYS(addr) >> PAGE_SIZE_SHIFT;

    ASSERT(page_mgr->pages[pfn].flags == PAGE_HEAD);
    ASSERT(page_mgr->pages[pfn].count == pages);

    uint64_t ref_count;
    ref_count = page_reference_dec_lock(pfn);
    if (ref_count == 1)
    {
        bitmap_set(&page_mgr->bitmap, pfn, 1, pages);
        page_mgr->pages[pfn].flags = 0;
        page_mgr->pages[pfn].count = 0;
    }
    return;
}

void free_pages(void *addr, size_t pages)
{
    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    spin_lock(&page_mgr->lock);
    free_pages_lock(addr, pages);
    spin_unlock(&page_mgr->lock);

    return;
}
