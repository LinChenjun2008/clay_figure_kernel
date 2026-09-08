// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <efi.h>
#include <mem/page.h>
#include <mem/struct.h>
#include <panic.h>
#include <print.h>
#include <sync/atomic.h>
#include <sync/spinlock.h>
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

    phys_addr_t curr_start = 0;
    uint64_t    curr_pages = 0;
    size_t      curr_pfn   = 0;

    struct efi_memory_descriptor *mem_desc = NULL;

    int desc_count = efi_mem_desc_count(memmap);

    int i;

    for (i = 0; i < desc_count; i++)
    {
        mem_desc  = read_efi_mem_desc(memmap, i);
        curr_type = get_page_type(mem_desc->type);

        curr_start = mem_desc->physical_start;
        curr_pages = mem_desc->number_of_pages;
        curr_pfn   = ADDR_TO_PFN(curr_start);
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
        struct page *page     = &page_mgr->pages[i];
        page->reference_count = 0;
        page->flags           = 0;
        page->count           = 0;
        init_spinlock(&page->lock);
    }
    return;
}

void page_struct_lock(size_t pfn)
{
    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    if (pfn > page_mgr->max_pfn)
    {
        return;
    }
    spin_lock(&page_mgr->pages[pfn].lock);
    return;
}

void page_struct_unlock(size_t pfn)
{
    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    if (pfn > page_mgr->max_pfn)
    {
        return;
    }
    spin_unlock(&page_mgr->pages[pfn].lock);
    return;
}

void page_reference_inc_lock(size_t pfn)
{
    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    if (pfn > page_mgr->max_pfn)
    {
        return;
    }
    page_mgr->pages[pfn].reference_count++;
    return;
}

uint32_t page_reference_dec_lock(size_t pfn)
{
    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    if (pfn > page_mgr->max_pfn)
    {
        return -1;
    }
    return page_mgr->pages[pfn].reference_count--;
}

uint32_t page_reference_read_lock(size_t pfn)
{
    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    if (pfn > page_mgr->max_pfn)
    {
        return -1;
    }
    return page_mgr->pages[pfn].reference_count;
}

void page_reference_inc(size_t pfn)
{
    page_struct_lock(pfn);
    page_reference_inc_lock(pfn);
    page_struct_unlock(pfn);
    return;
}

uint32_t page_reference_dec(size_t pfn)
{
    page_struct_lock(pfn);
    uint32_t ret = page_reference_dec_lock(pfn);
    page_struct_unlock(pfn);
    return ret;
}

uint32_t page_reference_read(size_t pfn)
{
    page_struct_lock(pfn);
    uint32_t ret = page_reference_read_lock(pfn);
    page_struct_unlock(pfn);
    return ret;
}

void *allocate_pages(size_t pages)
{
    if (pages > MAX_ALLOCATE_PAGES)
    {
        return NULL;
    }

    void *ret = NULL;

    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    spin_lock(&page_mgr->lock);
    size_t pfn = bitmap_find(&page_mgr->bitmap, 1, pages);
    if (pfn != -1UL)
    {
        bitmap_set(&page_mgr->bitmap, pfn, 0, pages);
    }
    spin_unlock(&page_mgr->lock);
    if (pfn == -1UL)
    {
        goto fail;
    }

    struct page *head_page = &page_mgr->pages[pfn];
    init_spinlock(&head_page->lock);

    head_page->reference_count = 1;
    head_page->flags |= PAGE_HEAD;
    head_page->count = pages;

    ret = PHYS_TO_VIRT(PFN_TO_ADDR(pfn));

fail:
    return ret;
}

void *allocate_a_page(void)
{
    return allocate_pages(1);
}

size_t free_pages(void *addr, size_t pages)
{
    if (pages > MAX_ALLOCATE_PAGES)
    {
        return 0;
    }

    if (addr == NULL)
    {
        printk(MSG_WARN "free_pages: Free null point.\n");
        return 0;
    }
    ASSERT(((uintptr_t)addr & (PG_SIZE - 1)) == 0);


    struct system_info *system_info = get_task_mgr()->system_info;
    struct page_mgr    *page_mgr    = system_info->page_mgr;

    size_t pfn = ADDR_TO_PFN(VIRT_TO_PHYS(addr));

    page_struct_lock(pfn);
    struct page *head_page = &page_mgr->pages[pfn];

    uint32_t ref_count;
    ref_count = page_reference_dec_lock(pfn);

    ASSERT(head_page->flags & PAGE_HEAD);
    ASSERT(head_page->count == pages);

    // dec之前只有一个引用,释放页
    if (ref_count == 1)
    {
        ASSERT(head_page->reference_count == 0);
        head_page->flags &= ~PAGE_HEAD;
        head_page->count = 0;
    }
    page_struct_unlock(pfn);

    if (ref_count == 1)
    {
        spin_lock(&page_mgr->lock);
        bitmap_set(&page_mgr->bitmap, pfn, 1, pages);
        spin_unlock(&page_mgr->lock);
    }
    return pages;
}

size_t free_a_page(void *addr)
{
    return free_pages(addr, 1);
}
