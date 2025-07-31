// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/spinlock.h> // spinlock
#include <lib/bitmap.h>      // bitmap
#include <mem/allocator.h>   // kmalloc
#include <mem/page.h>        // previous
#include <std/string.h>      // memset

typedef enum
{
    FREE_MEMORY = 1,
    RESERVED_MEMORY,
    ACPI_MEMORY,
    ACPI_MEMORY_NVS,
    UNUSEABLE_MEMORY,
    MAX_MEMORY_TYPE,
} memory_type_t;

typedef enum
{
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef struct
{
    uint32_t   Type;
    phy_addr_t PhysicalStart;
    addr_t     VirtualStart;
    uint64_t   NumberOfPages;
    uint64_t   Attribute;
} ALIGNED(16) EFI_MEMORY_DESCRIPTOR;

PRIVATE struct
{
    bitmap_t   page_bitmap;
    spinlock_t lock;
    size_t     mem_size;
    size_t     total_pages;      // 总页数
    size_t     total_free_pages; // 总空闲页数
    size_t     free_pages;       // 当前空闲页数
} mem;

/**
 * @brief 用于页分配的位图,bit为1表示对应的页空闲
 */
PRIVATE uint8_t page_bitmap_map[PAGE_BITMAP_BYTES_LEN];

PRIVATE size_t page_size_round_up(addr_t page_addr)
{
    return DIV_ROUND_UP(page_addr, PG_SIZE);
}

PRIVATE size_t page_size_round_down(addr_t page_addr)
{
    return page_addr / PG_SIZE;
}

PRIVATE memory_type_t memory_type(EFI_MEMORY_TYPE efi_type)
{
    switch (efi_type)
    {
        case EfiConventionalMemory:
        case EfiBootServicesCode:
        case EfiBootServicesData:
        case EfiLoaderCode:
            return FREE_MEMORY;
        case EfiLoaderData:
        case EfiRuntimeServicesCode:
        case EfiRuntimeServicesData:
        case EfiMemoryMappedIO:
        case EfiMemoryMappedIOPortSpace:
        case EfiPalCode:
        case EfiReservedMemoryType:
            return RESERVED_MEMORY;
        case EfiACPIReclaimMemory:
            return ACPI_MEMORY;
        case EfiACPIMemoryNVS:
            return ACPI_MEMORY_NVS;
        case EfiUnusableMemory:
        case EfiMaxMemoryType:
            return UNUSEABLE_MEMORY;
        default:
            return MAX_MEMORY_TYPE;
    }
    return MAX_MEMORY_TYPE;
}

PRIVATE const char *memory_type_str[] = {
    "Invaild",         "Free memory",      "Reserved", "ACPI memory",
    "ACPI memory NVS", "Unuseable memory", "Invaild",
};

PUBLIC void mem_page_init(void)
{
    mem.page_bitmap.map            = page_bitmap_map;
    mem.page_bitmap.btmp_bytes_len = PAGE_BITMAP_BYTES_LEN;
    mem.mem_size                   = 0;
    mem.total_pages                = 0;
    mem.free_pages                 = 0;
    init_bitmap(&mem.page_bitmap);
    init_spinlock(&mem.lock);
    EFI_MEMORY_DESCRIPTOR *efi_memory_desc =
        (EFI_MEMORY_DESCRIPTOR *)BOOT_INFO->memory_map.buffer;

    size_t map_size              = BOOT_INFO->memory_map.map_size;
    size_t desc_size             = BOOT_INFO->memory_map.descriptor_size;
    int    number_of_memory_desc = map_size / desc_size;


    // mem_xxx - 同类内存块的起始地址,大小,结束地址和类型
    phy_addr_t    mem_start = 0;
    phy_addr_t    mem_end   = 0;
    size_t        mem_size  = 0;
    memory_type_t mem_type  = MAX_MEMORY_TYPE;

    // curr_xxx - 当前内存块(efi_memory_desc[i])的起始地址,大小,结束地址和类型
    phy_addr_t    curr_start = 0;
    phy_addr_t    curr_end   = 0;
    size_t        curr_size  = 0;
    memory_type_t curr_type  = memory_type(efi_memory_desc[0].Type);

    size_t bit_start = 0;
    size_t bit_end   = 0;
    size_t bit_size  = 0;

    int i, j;
    for (i = 0; i < number_of_memory_desc; i++)
    {
        curr_start = efi_memory_desc[i].PhysicalStart;
        curr_size  = (efi_memory_desc[i].NumberOfPages << 12);
        curr_end   = curr_start + curr_size;

        mem_start = curr_start;
        mem_size  = curr_size;
        mem_end   = curr_end;

        curr_type = memory_type(efi_memory_desc[i].Type);

        // 类型相同的相邻内存空间合并处理,防止内存浪费
        // 当curr_type和next_type不同时,对类型为this_type的内存空间进行合并
        for (j = i + 1; j < number_of_memory_desc; j++)
        {
            curr_start = efi_memory_desc[j].PhysicalStart;
            curr_size  = (efi_memory_desc[j].NumberOfPages << 12);
            curr_end   = curr_start + curr_size;

            // 不连续 - 无法合并
            if (mem_end < curr_start)
            {
                break;
            }
            mem_type = memory_type(efi_memory_desc[j].Type);
            if (mem_type != curr_type)
            {
                break;
            }
            mem_size += curr_size;
            mem_end += curr_size;
            i = j;
        }
        PR_LOG(
            0,
            "From %p to %p: size: %8d KiB Type: %s.\n",
            mem_start,
            mem_end,
            mem_size >> 10,
            memory_type_str[curr_type]
        );
        if (curr_type == FREE_MEMORY)
        {
            mem.mem_size += mem_size;
            bit_start = page_size_round_up(mem_start);
            bit_end   = page_size_round_down(mem_end);
            bit_size  = bit_end - bit_start;
            if (bit_end > bit_start)
            {
                mem.total_free_pages += bit_size;
                mem.free_pages = mem.total_free_pages;
            }
            uint64_t bit_index;
            for (bit_index = bit_start; bit_index < bit_end; bit_index++)
            {
                bitmap_set(&mem.page_bitmap, bit_index, 1);
            }
        }
        mem.total_pages += bit_size;
        mem_start = 0;
        mem_end   = 0;
        mem_size  = 0;

        bit_start = 0;
        bit_end   = 0;
        bit_size  = 0;
    }
    PR_LOG(
        LOG_INFO,
        "Total Page(s): %d (Free: %d).\n",
        mem.total_pages,
        mem.total_free_pages
    );
    PR_LOG(
        LOG_INFO,
        "Mem Size: %d KiB(%d MiB), Free size: %d KiB(%d MiB) "
        "(waste: %d KiB).\n",
        mem.mem_size / 1024,
        mem.mem_size / (1024 * 1024),
        mem.total_free_pages * 2048,
        mem.total_free_pages * 2,
        (mem.mem_size - mem.total_free_pages * PG_SIZE) / 1024
    );
    if (mem.total_pages / 8 <= PAGE_BITMAP_BYTES_LEN)
    {
        mem.page_bitmap.btmp_bytes_len = mem.total_pages / 8;
    }
    // 剔除被占用的内存(0 - 6M)
    for (i = 0; i < 3; i++)
    {
        bitmap_set(&mem.page_bitmap, i, 0);
        mem.total_free_pages--;
    }
    return;
}

PUBLIC size_t get_total_free_pages(void)
{
    return mem.total_free_pages;
}

PUBLIC status_t alloc_physical_page(uint64_t number_of_pages, void *addr)
{
    ASSERT(addr != NULL);
    ASSERT(number_of_pages != 0);
    if (mem.free_pages < number_of_pages)
    {
        *(void **)addr = NULL;
        PR_LOG(
            LOG_WARN,
            "No free pages (%d < %d).\n",
            mem.free_pages,
            number_of_pages
        );
        return K_NOMEM;
    }
    spinlock_lock(&mem.lock);
    status_t status = alloc_physical_page_sub(number_of_pages, addr);
    spinlock_unlock(&mem.lock);
    return status;
}

PUBLIC status_t alloc_physical_page_sub(uint64_t number_of_pages, void *addr)
{
    ASSERT(addr != NULL);
    ASSERT(number_of_pages != 0);
    uint32_t index;
    status_t status;
    status = bitmap_alloc(&mem.page_bitmap, 1, number_of_pages, &index);
    if (ERROR(status))
    {
        PR_LOG(LOG_ERROR, "Out of Memory: %d.\n", status);
        return K_NOMEM;
    }
    phy_addr_t paddr = 0;

    uint64_t i;
    for (i = index; i < index + number_of_pages; i++)
    {
        bitmap_set(&mem.page_bitmap, i, 0);
    }
    mem.free_pages -= number_of_pages;
    paddr               = (0UL + (phy_addr_t)index * PG_SIZE);
    *(phy_addr_t *)addr = paddr;

    // memset(PHYS_TO_VIRT(paddr), 0, number_of_pages * PG_SIZE);
    return K_SUCCESS;
}

PUBLIC void free_physical_page(void *addr, uint64_t number_of_pages)
{
    ASSERT(number_of_pages != 0);
    ASSERT(addr != NULL && ((((phy_addr_t)addr) & 0x1fffff) == 0));
    spinlock_lock(&mem.lock);
    phy_addr_t i;
    for (i = (phy_addr_t)addr / PG_SIZE;
         i < (phy_addr_t)addr / PG_SIZE + number_of_pages;
         i++)
    {
        bitmap_set(&mem.page_bitmap, i, 1);
    }
    mem.free_pages += number_of_pages;
    spinlock_unlock(&mem.lock);
    return;
}


PUBLIC uint64_t *pml4t_entry(void *pml4t, void *vaddr)
{
    return (uint64_t *)pml4t + GET_FIELD((addr_t)vaddr, ADDR_PML4T_INDEX);
}

PUBLIC uint64_t *pdpt_entry(void *pml4t, void *vaddr)
{
    return (uint64_t *)(*(uint64_t *)PHYS_TO_VIRT(pml4t_entry(pml4t, vaddr)) &
                        ~0xfff) +
           GET_FIELD((addr_t)vaddr, ADDR_PDPT_INDEX);
}

PUBLIC uint64_t *pdt_entry(void *pml4t, void *vaddr)
{
    return (uint64_t *)(*(uint64_t *)PHYS_TO_VIRT(pdpt_entry(pml4t, vaddr)) &
                        ~0xfff) +
           GET_FIELD((addr_t)vaddr, ADDR_PDT_INDEX);
}

PUBLIC void *to_physical_address(void *pml4t, void *vaddr)
{
    uint64_t *v_pml4t, *v_pml4e;
    uint64_t *pdpt, *v_pdpte, *pdpte;
    uint64_t *pdt, *v_pde, *pde;
    v_pml4t = PHYS_TO_VIRT(pml4t);
    v_pml4e = v_pml4t + GET_FIELD((addr_t)vaddr, ADDR_PML4T_INDEX);
    if (!(*v_pml4e & PG_P))
    {
        PR_LOG(LOG_WARN, "vaddr pml4e not exist: %p\n", vaddr);
        return NULL;
    }
    pdpt    = (uint64_t *)(*v_pml4e & (~0xfff));
    pdpte   = pdpt + GET_FIELD((addr_t)vaddr, ADDR_PDPT_INDEX);
    v_pdpte = PHYS_TO_VIRT(pdpte);
    if (!(*v_pdpte & PG_P))
    {
        PR_LOG(LOG_WARN, "vaddr pdpte not exist: %p\n", vaddr);
        return NULL;
    }
    pdt   = (uint64_t *)(*v_pdpte & (~0xfff));
    pde   = pdt + GET_FIELD((addr_t)vaddr, ADDR_PDT_INDEX);
    v_pde = PHYS_TO_VIRT(pde);
    if (!(*v_pde & PG_P))
    {
        PR_LOG(LOG_WARN, "vaddr pde not exist: %p\n", vaddr);
        return NULL;
    }
    return (void *)((*v_pde & ~0xfff) + GET_FIELD((addr_t)vaddr, ADDR_OFFSET));
}

PUBLIC void page_map(uint64_t *pml4t, void *paddr, void *vaddr)
{
    paddr = (void *)((phy_addr_t)paddr & ~(PG_SIZE - 1));
    vaddr = (void *)((addr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t, *v_pml4e;
    uint64_t *v_pdpt, *pdpt, *v_pdpte, *pdpte;
    uint64_t *v_pdt, *pdt, *v_pde, *pde;
    v_pml4t = PHYS_TO_VIRT(pml4t);
    v_pml4e = v_pml4t + GET_FIELD((addr_t)vaddr, ADDR_PML4T_INDEX);
    status_t status;
    if (!(*v_pml4e & PG_P))
    {
        status = kmalloc(PT_SIZE, 0, PT_SIZE, &v_pdpt);
        ASSERT(!ERROR(status));
        UNUSED(status);
        pdpt = VIRT_TO_PHYS(v_pdpt);
        memset(v_pdpt, 0, PT_SIZE);
        *v_pml4e = (uint64_t)pdpt | PG_US_U | PG_RW_W | PG_P;
    }
    pdpt    = (uint64_t *)(*v_pml4e & (~0xfff));
    pdpte   = pdpt + GET_FIELD((addr_t)vaddr, ADDR_PDPT_INDEX);
    v_pdpte = PHYS_TO_VIRT(pdpte);
    if (!(*v_pdpte & PG_P))
    {
        status = kmalloc(PT_SIZE, 0, PT_SIZE, &v_pdt);
        ASSERT(!ERROR(status));
        UNUSED(status);
        pdt = VIRT_TO_PHYS(v_pdt);
        memset(v_pdt, 0, PT_SIZE);
        *v_pdpte = (uint64_t)pdt | PG_US_U | PG_RW_W | PG_P;
    }
    pdt    = (uint64_t *)(*v_pdpte & (~0xfff));
    pde    = pdt + GET_FIELD((addr_t)vaddr, ADDR_PDT_INDEX);
    v_pde  = PHYS_TO_VIRT(pde);
    *v_pde = (uint64_t)paddr | PG_DEFAULT_FLAGS;
    return;
}

PUBLIC void page_unmap(uint64_t *pml4t, void *vaddr)
{
    vaddr = (void *)((addr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t, *v_pml4e;
    uint64_t *pdpt, *v_pdpte, *pdpte;
    uint64_t *pdt, *v_pde, *pde;
    v_pml4t = PHYS_TO_VIRT(pml4t);
    v_pml4e = v_pml4t + GET_FIELD((addr_t)vaddr, ADDR_PML4T_INDEX);
    ASSERT(*v_pml4e & PG_P);

    pdpt    = (uint64_t *)(*v_pml4e & (~0xfff));
    pdpte   = pdpt + GET_FIELD((addr_t)vaddr, ADDR_PDPT_INDEX);
    v_pdpte = PHYS_TO_VIRT(pdpte);
    ASSERT(*v_pdpte & PG_P);

    pdt   = (uint64_t *)(*v_pdpte & (~0xfff));
    pde   = pdt + GET_FIELD((addr_t)vaddr, ADDR_PDT_INDEX);
    v_pde = PHYS_TO_VIRT(pde);
    ASSERT(*v_pde & PG_P);
    *v_pde &= ~PG_P;
    return;
}

PUBLIC void set_page_flags(uint64_t *pml4t, void *vaddr, uint64_t flags)
{
    vaddr = (void *)((addr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t, *v_pml4e;
    uint64_t *pdpt, *v_pdpte, *pdpte;
    uint64_t *pdt, *v_pde, *pde;
    v_pml4t = PHYS_TO_VIRT(pml4t);
    v_pml4e = v_pml4t + GET_FIELD((addr_t)vaddr, ADDR_PML4T_INDEX);
    ASSERT(*v_pml4e & PG_P);

    pdpt    = (uint64_t *)(*v_pml4e & (~0xfff));
    pdpte   = pdpt + GET_FIELD((addr_t)vaddr, ADDR_PDPT_INDEX);
    v_pdpte = PHYS_TO_VIRT(pdpte);
    ASSERT(*v_pdpte & PG_P);

    pdt   = (uint64_t *)(*v_pdpte & (~0xfff));
    pde   = pdt + GET_FIELD((addr_t)vaddr, ADDR_PDT_INDEX);
    v_pde = PHYS_TO_VIRT(pde);
    ASSERT(*v_pde & PG_P);
    *v_pde = (*v_pde & ~0xfff) | flags;
    return;
}