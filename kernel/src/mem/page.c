// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>
#include <asm/sync/spinlock.h>

#include <efi.h>
#include <lib/free_table.h>
#include <mem.h>
#include <mem/struct.h>
#include <print.h>
#include <std/string.h>
#include <sync/atomic.h>


static struct page_mgr *pg_allocator;

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

// 进行预处理(剔除低于1MiB的内存块)
static void efi_mem_desc_preprocess(struct memory_map *memmap)
{
    enum mm_type curr_type = MAX_MM_TYPE;

    uintptr_t curr_start = 0;
    uintptr_t curr_end   = 0;
    size_t    curr_size  = 0;
    uint64_t  curr_pages = 0;

    struct efi_memory_descriptor *mem_desc = NULL;

    int desc_count = efi_mem_desc_count(memmap);
    int i;
    for (i = 0; i < desc_count; i++)
    {
        mem_desc  = read_efi_mem_desc(memmap, i);
        curr_type = get_page_type(mem_desc->type);

        curr_start = mem_desc->physical_start;
        curr_pages = mem_desc->number_of_pages;
        curr_size  = (curr_pages << 12);
        curr_end   = curr_start + curr_size;

        // 只有类型为MM_TYPE_FREE的需要处理
        if (curr_type != MM_TYPE_FREE)
        {
            continue;
        }

        // 全部位于可用范围内
        if (curr_start >= 0x100000)
        {
            continue;
        }

        // 全部位于保留范围内
        if (curr_end <= 0x100000)
        {
            // 标记为不可用
            mem_desc->type = EFI_MAX_MEMORY_TYPE;
            continue;
        }

        // curr_start < 0x100000 && curr_end > 0x100000

        curr_start = 0x100000;
        curr_size  = curr_end - curr_start;
        curr_pages = curr_size >> PAGE_SIZE_SHIFT;

        mem_desc->physical_start  = curr_start;
        mem_desc->number_of_pages = curr_pages;
    }
    return;
}

// 获取空闲页个数
static size_t calculate_free_pages(struct memory_map *memmap)
{
    enum mm_type curr_type = MAX_MM_TYPE;

    size_t pages = 0;

    struct efi_memory_descriptor *mem_desc = NULL;

    int desc_count = efi_mem_desc_count(memmap);
    int i;
    for (i = 0; i < desc_count; i++)
    {
        mem_desc  = read_efi_mem_desc(memmap, i);
        curr_type = get_page_type(mem_desc->type);

        if (curr_type != MM_TYPE_FREE)
        {
            continue;
        }

        pages += mem_desc->number_of_pages;
    }
    return pages;
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

// 利用memmap分配内存
// 分配的内存不会释放
static void *memmap_alloc_pages(struct memory_map *memmap, uint64_t pages)
{
    enum mm_type curr_type = MAX_MM_TYPE;

    void *ret = NULL;

    struct efi_memory_descriptor *mem_desc = NULL;

    int desc_count = efi_mem_desc_count(memmap);
    int i;
    for (i = 0; i < desc_count; i++)
    {
        mem_desc  = read_efi_mem_desc(memmap, i);
        curr_type = get_page_type(mem_desc->type);

        if (curr_type != MM_TYPE_FREE)
        {
            continue;
        }
        if (mem_desc->number_of_pages < pages)
        {
            continue;
        }
        // mem_desc->number_of_pages >= pages
        ret = (void *)mem_desc->physical_start;

        mem_desc->physical_start += (pages << PAGE_SIZE_SHIFT);
        mem_desc->number_of_pages -= pages;
        return PHYS_TO_VIRT(ret);
    }
    return NULL;
}

static void free_all_pages(struct memory_map *memmap)
{
    enum mm_type                  curr_type = MAX_MM_TYPE;
    struct efi_memory_descriptor *mem_desc  = NULL;

    void  *addr;
    size_t pages;
    int    i;
    for (i = 0; i < efi_mem_desc_count(memmap); i++)
    {
        mem_desc  = read_efi_mem_desc(memmap, i);
        curr_type = get_page_type(mem_desc->type);

        if (curr_type != MM_TYPE_FREE)
        {
            continue;
        }
        addr  = PHYS_TO_VIRT(mem_desc->physical_start);
        pages = mem_desc->number_of_pages;
        free_pages(addr, pages);
    }
    return;
}

void pg_allocator_init(struct boot_info *boot_info)
{
    efi_mem_desc_preprocess(&boot_info->memory_map);

    // free_table
    size_t total_pages  = calculate_free_pages(&boot_info->memory_map);
    int    total_blocks = (total_pages / 2) + 1;
    size_t table_size   = total_blocks * sizeof(struct free_block);

    // pg_ref_count
    size_t max_pfn           = calculate_max_pfn(&boot_info->memory_map);
    size_t pg_ref_count_size = sizeof(pg_allocator->pg_ref_count[0]);
    size_t pg_ref_size       = pg_ref_count_size * (max_pfn + 1);

    // pg_allocator
    size_t           allocator_size = sizeof(struct page_mgr);
    size_t           total_size     = allocator_size + table_size + pg_ref_size;
    size_t           pages = (total_size + PG_SIZE - 1) >> PAGE_SIZE_SHIFT;
    struct page_mgr *allocator;
    allocator = memmap_alloc_pages(&boot_info->memory_map, pages);
    if (allocator == NULL)
    {
        printk(MSG_ERR "page_allocator_init: failed to allocate table.\n");
        return;
    }
    memset(allocator, 0, allocator_size);

    pg_allocator             = allocator;
    uintptr_t allocator_base = (uintptr_t)allocator;
    uintptr_t table_base     = allocator_base + allocator_size;
    uintptr_t pg_ref_base    = table_base + table_size;

    init_free_table(&pg_allocator->pg_map, (void *)table_base, total_blocks);

    allocator->pg_ref_count = (void *)pg_ref_base;
    size_t i;
    for (i = 0; i <= max_pfn; i++)
    {
        atomic_set(&allocator->pg_ref_count[i], 0);
    }

    free_all_pages(&boot_info->memory_map);
    printk("pg_allocator_init: total memory: %d MiB.\n", total_pages >> 10);
    return;
}

void free_pages(void *addr, size_t pages)
{
    if (addr == NULL)
    {
        printk(MSG_WARN "free_pages: Free null point.\n");
        return;
    }
    ASSERT(((uintptr_t)addr & (PG_SIZE - 1)) == 0);

    int    status = 0;
    size_t size   = pages << PAGE_SIZE_SHIFT;

    status = free_table_add(&pg_allocator->pg_map, (uint64_t)addr, size);
    if (status < 0)
    {
        printk(MSG_ERR "free_pages: free page failed (%p, %d).\n", addr, pages);
    }
    return;
}

void *allocate_pages(size_t pages)
{
    uint64_t ret  = 0;
    size_t   size = pages << PAGE_SIZE_SHIFT;
    ret           = free_table_allocate(&pg_allocator->pg_map, size);
    if (ret == -1UL)
    {
        return NULL;
    }
    return (void *)ret;
}
