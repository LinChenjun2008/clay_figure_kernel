// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>
#include <asm/sync/spinlock.h>
#include <asm/utils.h>

#include <efi.h>
#include <lib/free_table.h>
#include <mem.h>
#include <print.h>
#include <std/string.h>

struct page_allocator
{
    struct free_table pg_map;
    struct spinlock   lock;
};

static struct page_allocator *pg_allocator;

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
    size_t total_pages    = calculate_free_pages(&boot_info->memory_map);
    size_t allocator_size = sizeof(struct page_allocator);
    int    total_blocks   = (total_pages / 2) + 1;
    size_t table_size     = total_blocks * sizeof(struct free_block);

    size_t total_size = allocator_size + table_size;
    size_t pages      = (total_size + PG_SIZE - 1) >> PAGE_SIZE_SHIFT;
    struct page_allocator *allocator;
    allocator = memmap_alloc_pages(&boot_info->memory_map, pages);
    if (allocator == NULL)
    {
        printk(MSG_ERR "page_allocator_init: failed to allocate table.\n");
        return;
    }
    memset(allocator, 0, allocator_size);

    pg_allocator = allocator;
    init_free_table(&pg_allocator->pg_map, pg_allocator + 1, total_blocks);

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