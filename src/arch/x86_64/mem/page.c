// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <io.h>            // get_cr2,get_cr3
#include <lib/bitmap.h>    // bitmap
#include <mem/allocator.h> // kmalloc
#include <mem/page.h>      // previous
#include <std/string.h>    // memset
#include <sync/spinlock.h> // spinlock

// do_page_fault
#include <intr.h>      // register_handler
#include <task/task.h> // task_struct

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
    uint32_t  Type;
    uintptr_t PhysicalStart;
    uintptr_t VirtualStart;
    uint64_t  NumberOfPages;
    uint64_t  Attribute;
} ALIGNED(16) EFI_MEMORY_DESCRIPTOR;

PRIVATE struct
{
    mm_struct_t pages;
    spinlock_t  lock;
    size_t      mem_size;
    size_t      total_pages;      // 总页数
    size_t      total_free_pages; // 总空闲页数
    size_t      free_pages;       // 当前空闲页数
} mem_man;

PRIVATE mm_block_t page_mm_blocks[PAGE_BLOCKS];

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

// PRIVATE const char *memory_type_str[] = {
//     "Invaild",         "Free memory",      "Reserved", "ACPI memory",
//     "ACPI memory NVS", "Unuseable memory", "Invaild",
// };

PRIVATE void do_page_fault(intr_stack_t *stack)
{
    task_struct_t *task          = running_task();
    uintptr_t      fault_address = get_cr2();
    uintptr_t      cr3           = get_cr3();
    uint64_t       error_code    = stack->error_code;

    uintptr_t fault_page = fault_address & ~(PG_SIZE - 1);
    // 内核任务 - 错误
    if (cr3 == KERNEL_PAGE_DIR_TABLE_POS)
    {
        pr_log(LOG_ERROR, "Kernel task page fault.\n");
        default_irq_handler(stack);
    }

    // 未分配地址 - 错误
    if (!mm_find(&task->mm_using, fault_address))
    {
        pr_log(LOG_ERROR, "Page not allocated.\n");
        default_irq_handler(stack);
    }

    //  页已存在而引发的异常
    if (error_code & PG_P)
    {
        pr_log(LOG_ERROR, "Page existed.\n");
        default_irq_handler(stack);
    }
    uintptr_t paddr;
    status_t  status = alloc_physical_page(1, &paddr);
    if (ERROR(status))
    {
        PANIC(ERROR(status), "Out of memory.\n");
        default_irq_handler(stack);
    }
    mm_add_range(&task->mm_pages, paddr, PG_SIZE);
    page_map(task->page_dir, (void *)paddr, (void *)fault_page, 1);
    page_table_activate(task);
    return;
}

PUBLIC void mem_page_init(void)
{
    mm_struct_init(&mem_man.pages, page_mm_blocks, PAGE_BLOCKS);
    mem_man.mem_size    = 0;
    mem_man.total_pages = 0;
    mem_man.free_pages  = 0;
    init_spinlock(&mem_man.lock);

    EFI_MEMORY_DESCRIPTOR *efi_memory_desc =
        (EFI_MEMORY_DESCRIPTOR *)BOOT_INFO->memory_map.buffer;

    size_t map_size              = BOOT_INFO->memory_map.map_size;
    size_t desc_size             = BOOT_INFO->memory_map.descriptor_size;
    int    number_of_memory_desc = map_size / desc_size;

    // curr_xxx - 当前内存块(efi_memory_desc[i])的起始地址,大小,结束地址和类型
    uintptr_t     curr_start = 0;
    uintptr_t     curr_end   = 0;
    size_t        curr_size  = 0;
    uint64_t      curr_pages = 0;
    memory_type_t curr_type  = memory_type(efi_memory_desc[0].Type);

    int i;
    for (i = 0; i < number_of_memory_desc; i++)
    {
        curr_start = efi_memory_desc[i].PhysicalStart;
        curr_pages = efi_memory_desc[i].NumberOfPages;
        curr_size  = (curr_pages << 12);
        curr_end   = curr_start + curr_size;

        curr_type = memory_type(efi_memory_desc[i].Type);

        mem_man.total_pages += curr_pages;
        mem_man.mem_size += curr_size;
        // PR_MSG(
        //     "From %p to %p: size: %8d KiB Type: %s.\n",
        //     curr_start,
        //     curr_end,
        //     curr_size >> 10,
        //     memory_type_str[curr_type]
        // );
        if (curr_type == FREE_MEMORY)
        {
            if (curr_end < 0x2000000)
            {
                continue;
            }
            if (curr_start < 0x2000000)
            {
                curr_start = 0x2000000;
                curr_size  = curr_end - curr_start;
                curr_pages = curr_size >> 12;
            }
            mm_add_range_sub(&mem_man.pages, curr_start, curr_size);
            mem_man.total_free_pages += curr_pages;
            mem_man.free_pages += curr_pages;
        }
    }
    // PR_LOG(
    //     LOG_INFO,
    //     "Total Page(s): %d (Free: %d).\n",
    //     mem_man.total_pages,
    //     mem_man.total_free_pages
    // );
    // PR_LOG(
    //     LOG_INFO,
    //     "Mem Size: %d KiB(%d MiB), Free size: %d KiB(%d MiB)\n",
    //     mem_man.mem_size / 1024,
    //     mem_man.mem_size / (1024 * 1024),
    //     mem_man.total_free_pages * 4,
    //     mem_man.total_free_pages * 4 / 1024
    // );
    register_handle(0x0e, do_page_fault);
    return;
}

PUBLIC size_t get_total_free_pages(void)
{
    return mem_man.total_free_pages;
}

PUBLIC status_t alloc_physical_page(uint64_t number_of_pages, void *addr)
{
    ASSERT(addr != NULL);
    ASSERT(number_of_pages != 0);
    if (mem_man.free_pages < number_of_pages)
    {
        return K_NOMEM;
    }
    spinlock_lock(&mem_man.lock);
    status_t status;
    status = mm_alloc(&mem_man.pages, number_of_pages * PG_SIZE, addr);
    if (ERROR(status))
    {
        PR_LOG(LOG_ERROR, "Out of Memory: %d.\n", status);
        return K_NOMEM;
    }
    mem_man.free_pages -= number_of_pages;
    spinlock_unlock(&mem_man.lock);
    return status;
}

PUBLIC status_t alloc_physical_page_sub(uint64_t number_of_pages, void *addr)
{
    ASSERT(addr != NULL);
    ASSERT(number_of_pages != 0);
    status_t status;
    status = mm_alloc_sub(&mem_man.pages, number_of_pages * PG_SIZE, addr);
    if (ERROR(status))
    {
        PR_LOG(LOG_ERROR, "Out of Memory: %d.\n", status);
        return K_NOMEM;
    }
    mem_man.free_pages -= number_of_pages;
    return K_SUCCESS;
}

PUBLIC void free_physical_page(void *addr, uint64_t number_of_pages)
{
    ASSERT(number_of_pages != 0);
    ASSERT(addr != NULL && ((((uintptr_t)addr) & (PG_SIZE - 1)) == 0));
    spinlock_lock(&mem_man.lock);
    mm_add_range(&mem_man.pages, (uintptr_t)addr, number_of_pages * PG_SIZE);
    mem_man.free_pages += number_of_pages;
    spinlock_unlock(&mem_man.lock);
    return;
}


PUBLIC uint64_t *pml4t_entry(void *pml4t, void *vaddr)
{
    return (uint64_t *)pml4t + GET_FIELD((uintptr_t)vaddr, ADDR_PML4T_INDEX);
}

PUBLIC uint64_t *pdpt_entry(void *pml4t, void *vaddr)
{
    return (uint64_t *)(*(uint64_t *)PHYS_TO_VIRT(pml4t_entry(pml4t, vaddr)) &
                        ~0xfff) +
           GET_FIELD((uintptr_t)vaddr, ADDR_PDPT_INDEX);
}

PUBLIC uint64_t *pdt_entry(void *pml4t, void *vaddr)
{
    return (uint64_t *)(*(uint64_t *)PHYS_TO_VIRT(pdpt_entry(pml4t, vaddr)) &
                        ~0xfff) +
           GET_FIELD((uintptr_t)vaddr, ADDR_PDT_INDEX);
}

PUBLIC uint64_t *pt_entry(void *pml4t, void *vaddr)
{
    return (uint64_t *)(*(uint64_t *)PHYS_TO_VIRT(pdt_entry(pml4t, vaddr)) &
                        ~0xfff) +
           GET_FIELD((uintptr_t)vaddr, ADDR_PT_INDEX);
}

PUBLIC void *to_physical_address(void *pml4t, void *vaddr)
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

PRIVATE void page_map_sub(uint64_t *pml4t, void *paddr, void *vaddr)
{
    paddr = (void *)((uintptr_t)paddr & ~(PG_SIZE - 1));
    vaddr = (void *)((uintptr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t, *v_pml4e;
    uint64_t *v_pdpt, *pdpt, *v_pdpte, *pdpte;
    uint64_t *v_pdt, *pdt, *v_pde, *pde;
    uint64_t *v_pt, *pt, *v_pte, *pte;

    v_pml4t = PHYS_TO_VIRT(pml4t);
    v_pml4e = v_pml4t + GET_FIELD((uintptr_t)vaddr, ADDR_PML4T_INDEX);
    status_t status;
    if (!(*v_pml4e & PG_P))
    {
        status = alloc_physical_page(1, &pdpt);
        ASSERT(!ERROR(status));
        UNUSED(status);
        v_pdpt = PHYS_TO_VIRT(pdpt);
        memset(v_pdpt, 0, PT_SIZE);
        *v_pml4e = (uintptr_t)pdpt | PG_US_U | PG_RW_W | PG_P;
    }
    pdpt    = (uint64_t *)(*v_pml4e & (~0xfff));
    pdpte   = pdpt + GET_FIELD((uintptr_t)vaddr, ADDR_PDPT_INDEX);
    v_pdpte = PHYS_TO_VIRT(pdpte);
    if (!(*v_pdpte & PG_P))
    {
        status = alloc_physical_page(1, &pdt);
        ASSERT(!ERROR(status));
        UNUSED(status);
        v_pdt = PHYS_TO_VIRT(pdt);
        memset(v_pdt, 0, PT_SIZE);
        *v_pdpte = (uintptr_t)pdt | PG_US_U | PG_RW_W | PG_P;
    }
    pdt   = (uint64_t *)(*v_pdpte & (~0xfff));
    pde   = pdt + GET_FIELD((uintptr_t)vaddr, ADDR_PDT_INDEX);
    v_pde = PHYS_TO_VIRT(pde);
    if (!(*v_pde & PG_P))
    {
        status = alloc_physical_page(1, &pt);
        ASSERT(!ERROR(status));
        UNUSED(status);
        v_pt = PHYS_TO_VIRT(pt);
        memset(v_pt, 0, PT_SIZE);
        *v_pde = (uintptr_t)pt | PG_US_U | PG_RW_W | PG_P;
    }
    pt     = (uint64_t *)(*v_pde & (~0xfff));
    pte    = pt + GET_FIELD((uintptr_t)vaddr, ADDR_PT_INDEX);
    v_pte  = PHYS_TO_VIRT(pte);
    *v_pte = (uintptr_t)paddr | PG_DEFAULT_FLAGS;
    return;
}

PUBLIC void page_map(uint64_t *pml4t, void *paddr, void *vaddr, uint64_t count)
{
    uint64_t i;
    for (i = 0; i < count; i++)
    {
        page_map_sub(
            pml4t,
            (void *)((uintptr_t)paddr + i * PG_SIZE),
            (void *)((uintptr_t)vaddr + i * PG_SIZE)
        );
    }
    return;
}

PRIVATE void page_unmap_sub(uint64_t *pml4t, void *vaddr)
{
    vaddr = (void *)((uintptr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t, *v_pml4e;
    uint64_t *pdpt, *v_pdpte, *pdpte;
    uint64_t *pdt, *v_pde, *pde;
    uint64_t *pt, *v_pte, *pte;

    v_pml4t = PHYS_TO_VIRT(pml4t);
    v_pml4e = v_pml4t + GET_FIELD((uintptr_t)vaddr, ADDR_PML4T_INDEX);
    ASSERT(*v_pml4e & PG_P);

    pdpt    = (uint64_t *)(*v_pml4e & (~0xfff));
    pdpte   = pdpt + GET_FIELD((uintptr_t)vaddr, ADDR_PDPT_INDEX);
    v_pdpte = PHYS_TO_VIRT(pdpte);
    ASSERT(*v_pdpte & PG_P);

    pdt   = (uint64_t *)(*v_pdpte & (~0xfff));
    pde   = pdt + GET_FIELD((uintptr_t)vaddr, ADDR_PDT_INDEX);
    v_pde = PHYS_TO_VIRT(pde);
    ASSERT(*v_pde & PG_P);

    pt    = (uint64_t *)(*v_pde & (~0xfff));
    pte   = pt + GET_FIELD((uintptr_t)vaddr, ADDR_PT_INDEX);
    v_pte = PHYS_TO_VIRT(pte);
    ASSERT(*v_pte & PG_P);
    *v_pte &= ~PG_P;
    return;
}

PUBLIC void page_unmap(uint64_t *pml4t, void *vaddr, uint64_t count)
{
    uint64_t i;
    for (i = 0; i < count; i++)
    {
        page_unmap_sub(pml4t, (void *)((uintptr_t)vaddr + i * PG_SIZE));
    }
    return;
}

PUBLIC void set_page_flags(uint64_t *pml4t, void *vaddr, uint64_t flags)
{
    vaddr = (void *)((uintptr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t, *v_pml4e;
    uint64_t *pdpt, *v_pdpte, *pdpte;
    uint64_t *pdt, *v_pde, *pde;
    uint64_t *pt, *v_pte, *pte;

    v_pml4t = PHYS_TO_VIRT(pml4t);
    v_pml4e = v_pml4t + GET_FIELD((uintptr_t)vaddr, ADDR_PML4T_INDEX);
    ASSERT(*v_pml4e & PG_P);

    pdpt    = (uint64_t *)(*v_pml4e & (~0xfff));
    pdpte   = pdpt + GET_FIELD((uintptr_t)vaddr, ADDR_PDPT_INDEX);
    v_pdpte = PHYS_TO_VIRT(pdpte);
    ASSERT(*v_pdpte & PG_P);

    pdt   = (uint64_t *)(*v_pdpte & (~0xfff));
    pde   = pdt + GET_FIELD((uintptr_t)vaddr, ADDR_PDT_INDEX);
    v_pde = PHYS_TO_VIRT(pde);
    ASSERT(*v_pde & PG_P);

    pt    = (uint64_t *)(*v_pde & (~0xfff));
    pte   = pt + GET_FIELD((uintptr_t)vaddr, ADDR_PT_INDEX);
    v_pte = PHYS_TO_VIRT(pte);
    ASSERT(*v_pte & PG_P);
    *v_pte = (*v_pte & ~0xfff) | flags;
    return;
}

PUBLIC void set_page_table(void *page_table_pos)
{
    set_cr3((uint64_t)page_table_pos);
    return;
}

PRIVATE void free_pt(uintptr_t pt)
{
    free_physical_page((void *)pt, 1);
    return;
}

PRIVATE void free_pdt(uintptr_t pdt)
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
    free_physical_page((void *)pdt, 1);
    return;
}

PRIVATE void free_pdpt(uintptr_t pdpt)
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
    free_physical_page((void *)pdpt, 1);
    return;
}

PUBLIC void free_page_table(uint64_t *pml4t)
{
    uint64_t *v_pml4t = PHYS_TO_VIRT(pml4t);

    int i;
    for (i = 0; i < 256; i++) // 仅限用户空间
    {
        if (v_pml4t[i] & PG_P)
        {
            free_pdpt(v_pml4t[i] & (~0xfff));
        }
    }
    free_physical_page((void *)pml4t, 1);
    return;
}