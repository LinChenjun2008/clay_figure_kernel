/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <lib/bitmap.h>      // bitmap
#include <mem/mem.h>         // pmalloc
#include <device/spinlock.h> // spinlock
#include <std/string.h>      // memset

#include <log.h>

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
} __attribute__((aligned(16))) EFI_MEMORY_DESCRIPTOR;

PRIVATE struct
{
    bitmap_t   page_bitmap;
    spinlock_t lock;
    size_t     mem_size;
    uint32_t   total_pages;
    uint32_t   free_pages;
} mem;

PRIVATE uint8_t page_bitmap_map[PAGE_BITMAP_BYTES_LEN];

PRIVATE size_t page_size_round_up(addr_t page_addr)
{
    return DIV_ROUND_UP(page_addr,PG_SIZE);
}

PRIVATE size_t page_size_round_down(addr_t page_addr)
{
    return page_addr / PG_SIZE;
}

PRIVATE memory_type_t memory_type(EFI_MEMORY_TYPE efi_type)
{
    switch(efi_type)
    {
        case EfiConventionalMemory:
        case EfiBootServicesCode:
        case EfiBootServicesData:
        case EfiLoaderCode:
            return FREE_MEMORY;
            break;
        case EfiLoaderData:
        case EfiRuntimeServicesCode:
        case EfiRuntimeServicesData:
        case EfiMemoryMappedIO:
        case EfiMemoryMappedIOPortSpace:
        case EfiPalCode:
        case EfiReservedMemoryType:
            return RESERVED_MEMORY;
            break;
        case EfiACPIReclaimMemory:
            return ACPI_MEMORY;
            break;
        case EfiACPIMemoryNVS:
            return ACPI_MEMORY_NVS;
            break;
        case EfiUnusableMemory:
        case EfiMaxMemoryType:
            return UNUSEABLE_MEMORY;
            break;
        default:
            return MAX_MEMORY_TYPE;
            break;
    }
    return MAX_MEMORY_TYPE;
}

PUBLIC void mem_init(void)
{
    mem.page_bitmap.map            = page_bitmap_map;
    mem.page_bitmap.btmp_bytes_len = PAGE_BITMAP_BYTES_LEN;
    mem.mem_size                   = 0;
    mem.total_pages                = 0;
    mem.free_pages                 = 0;
    init_bitmap(&mem.page_bitmap);
    init_spinlock(&mem.lock);
    int number_of_memory_desct;
    number_of_memory_desct = g_boot_info->memory_map.map_size
                           / g_boot_info->memory_map.descriptor_size;
    addr_t    max_address = 0;
    int i;
    for (i = 0;i < number_of_memory_desct;i++)
    {
        EFI_MEMORY_DESCRIPTOR *efi_memory_desc =
                          (EFI_MEMORY_DESCRIPTOR*)g_boot_info->memory_map.buffer;
        phy_addr_t start = efi_memory_desc[i].PhysicalStart;
        phy_addr_t end   = start + (efi_memory_desc[i].NumberOfPages << 12);
        max_address      = efi_memory_desc[i].PhysicalStart
                          + (efi_memory_desc[i].NumberOfPages << 12);
        if (memory_type(efi_memory_desc[i].Type) != FREE_MEMORY)
        {
            start = page_size_round_down(start);
            end   = page_size_round_up(end);
            uint64_t j;
            for(j = start;j < end;j++)
            {
                bitmap_set(&mem.page_bitmap,j,1);
            }
        }
        else
        {
            mem.mem_size += efi_memory_desc[i].NumberOfPages << 12;
            start = page_size_round_up(start);
            end   = page_size_round_down(end);
            if(end >= start)
            {
                mem.total_pages += end - start;
                mem.free_pages = mem.total_pages;
            }
        }
    }
    if (max_address / PG_SIZE < PAGE_BITMAP_BYTES_LEN)
    {
        mem.page_bitmap.btmp_bytes_len = max_address / PG_SIZE;
    }
    // 剔除被占用的内存(0 - 6M)
    for (i = 0;i < 3;i++)
    {
        bitmap_set(&mem.page_bitmap,i,1);
    }
    return;
}

PUBLIC size_t total_memory(void)
{
    return mem.mem_size;
}

PUBLIC uint32_t total_pages(void)
{
    return mem.total_pages;
}

PUBLIC uint32_t total_free_pages(void)
{
    return mem.free_pages;
}

PUBLIC status_t alloc_physical_page(uint64_t number_of_pages,void *addr)
{
    ASSERT(addr != NULL);
    ASSERT(number_of_pages != 0);
    spinlock_lock(&mem.lock);
    uint32_t index;
    status_t status = bitmap_alloc(&mem.page_bitmap,number_of_pages,&index);
    ASSERT(!ERROR(status));
    if (ERROR(status))
    {
        spinlock_unlock(&mem.lock);
        pr_log("\3 %s:Out of Memory.\n",__func__);
        return K_NOMEM;
    }
    phy_addr_t paddr = 0;

    uint64_t i;
    for (i = index;i < index + number_of_pages;i++)
    {
        bitmap_set(&mem.page_bitmap,i,1);
    }
    mem.free_pages -= number_of_pages;
    spinlock_unlock(&mem.lock);
    paddr = (0UL + (phy_addr_t)index * PG_SIZE);
    memset(KADDR_P2V(paddr),0,number_of_pages * PG_SIZE);

    *(phy_addr_t*)addr = paddr;
    return K_SUCCESS;
}

PUBLIC status_t init_alloc_physical_page(uint64_t number_of_pages,void *addr)
{
    ASSERT(addr != NULL);
    ASSERT(number_of_pages != 0);
    uint32_t index;
    status_t status = bitmap_alloc(&mem.page_bitmap,number_of_pages,&index);
    ASSERT(!ERROR(status));
    (void)status;
    phy_addr_t paddr = 0;

    uint64_t i;
    for (i = index;i < index + number_of_pages;i++)
    {
        bitmap_set(&mem.page_bitmap,i,1);
    }
    paddr = (0UL + (phy_addr_t)index * PG_SIZE);
    memset(KADDR_P2V(paddr),0,number_of_pages * PG_SIZE);

    *(phy_addr_t*)addr = paddr;
    return K_SUCCESS;
}

PUBLIC void free_physical_page(void *addr,uint64_t number_of_pages)
{
    ASSERT(number_of_pages != 0);
    ASSERT(addr != NULL && ((((phy_addr_t)addr) & 0x1fffff) == 0));
    spinlock_lock(&mem.lock);
    phy_addr_t i;
    for (i = (phy_addr_t)addr / PG_SIZE;
         i < (phy_addr_t)addr / PG_SIZE + number_of_pages;i++)
    {
        bitmap_set(&mem.page_bitmap,i,0);
    }
    mem.free_pages += number_of_pages;
    spinlock_unlock(&mem.lock);
    return;
}


PUBLIC uint64_t* pml4t_entry(void *pml4t,void *vaddr)
{
    return (uint64_t*)pml4t + GET_FIELD((addr_t)vaddr,ADDR_PML4T_INDEX);
}

PUBLIC uint64_t* pdpt_entry(void *pml4t,void *vaddr)
{
    return (uint64_t*)(*(uint64_t*)KADDR_P2V(pml4t_entry(pml4t,vaddr)) & ~0xfff)
            + GET_FIELD((addr_t)vaddr,ADDR_PDPT_INDEX);
}

PUBLIC uint64_t* pdt_entry(void *pml4t,void *vaddr)
{
    return (uint64_t*)(*(uint64_t*)KADDR_P2V(pdpt_entry(pml4t,vaddr)) & ~0xfff)
            + GET_FIELD((addr_t)vaddr,ADDR_PDT_INDEX);
}

PUBLIC void* to_physical_address(void *pml4t,void *vaddr)
{
    uint64_t *v_pml4t,*v_pml4e;
    uint64_t *pdpt,*v_pdpte,*pdpte;
    uint64_t *pdt,*v_pde,*pde;
    v_pml4t = KADDR_P2V(pml4t);
    v_pml4e = v_pml4t + GET_FIELD((addr_t)vaddr,ADDR_PML4T_INDEX);
    if (!(*v_pml4e & PG_P))
    {
        pr_log("\3 %s:vaddr pml4e not exist: %p\n",__func__,vaddr);
        return NULL;
    }
    pdpt = (uint64_t*)(*v_pml4e & (~0xfff));
    pdpte = pdpt + GET_FIELD((addr_t)vaddr,ADDR_PDPT_INDEX);
    v_pdpte = KADDR_P2V(pdpte);
    if (!(*v_pdpte & PG_P))
    {
        pr_log("\3 %s:vaddr pdpte not exist: %p\n",__func__,vaddr);
        return NULL;
    }
    pdt = (uint64_t*)(*v_pdpte & (~0xfff));
    pde = pdt + GET_FIELD((addr_t)vaddr,ADDR_PDT_INDEX);
    v_pde = KADDR_P2V(pde);
    if (!(*v_pde & PG_P))
    {
        pr_log("\3 %s:vaddr pde not exist: %p\n",__func__,vaddr);
        return NULL;
    }
    return (void*)((*v_pde & ~0xfff) + GET_FIELD((addr_t)vaddr,ADDR_OFFSET));
}

PUBLIC void page_map(uint64_t *pml4t,void *paddr,void *vaddr)
{
    paddr = (void*)((phy_addr_t)paddr & ~(PG_SIZE - 1));
    vaddr = (void*)(    (addr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t,*v_pml4e;
    uint64_t *v_pdpt,*pdpt,*v_pdpte,*pdpte;
    uint64_t *v_pdt,*pdt,*v_pde,*pde;
    v_pml4t = KADDR_P2V(pml4t);
    v_pml4e = v_pml4t + GET_FIELD((addr_t)vaddr,ADDR_PML4T_INDEX);
    status_t status;
    if (!(*v_pml4e & PG_P))
    {
        status = pmalloc(PT_SIZE,0,PT_SIZE,&pdpt);
        ASSERT(!ERROR(status));
        (void)status;
        v_pdpt = KADDR_P2V(pdpt);
        memset(v_pdpt,0,PT_SIZE);
        *v_pml4e = (uint64_t)pdpt | PG_US_U | PG_RW_W | PG_P;
    }
    pdpt = (uint64_t*)(*v_pml4e & (~0xfff));
    pdpte = pdpt + GET_FIELD((addr_t)vaddr,ADDR_PDPT_INDEX);
    v_pdpte = KADDR_P2V(pdpte);
    if (!(*v_pdpte & PG_P))
    {
        status = pmalloc(PT_SIZE,0,PT_SIZE,&pdt);
        ASSERT(!ERROR(status));
        (void)status;
        v_pdt = KADDR_P2V(pdt);
        memset(v_pdt,0,PT_SIZE);
        *v_pdpte = (uint64_t)pdt | PG_US_U | PG_RW_W | PG_P;
    }
    pdt = (uint64_t*)(*v_pdpte & (~0xfff));
    pde = pdt + GET_FIELD((addr_t)vaddr,ADDR_PDT_INDEX);
    v_pde = KADDR_P2V(pde);
    *v_pde = (uint64_t)paddr | PG_DEFAULT_FLAGS;
    return;
}

PUBLIC void page_unmap(uint64_t *pml4t,void *vaddr)
{
    vaddr = (void*)((addr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t,*v_pml4e;
    uint64_t *pdpt,*v_pdpte,*pdpte;
    uint64_t *pdt,*v_pde,*pde;
    v_pml4t = KADDR_P2V(pml4t);
    v_pml4e = v_pml4t + GET_FIELD((addr_t)vaddr,ADDR_PML4T_INDEX);
    ASSERT(*v_pml4e & PG_P);

    pdpt = (uint64_t*)(*v_pml4e & (~0xfff));
    pdpte = pdpt + GET_FIELD((addr_t)vaddr,ADDR_PDPT_INDEX);
    v_pdpte = KADDR_P2V(pdpte);
    ASSERT(*v_pdpte & PG_P);

    pdt = (uint64_t*)(*v_pdpte & (~0xfff));
    pde = pdt + GET_FIELD((addr_t)vaddr,ADDR_PDT_INDEX);
    v_pde = KADDR_P2V(pde);
    ASSERT(*v_pde & PG_P);
    *v_pde &= ~PG_P;
    return;
}

PUBLIC void set_page_flags(uint64_t *pml4t,void *vaddr,uint64_t flags)
{
    vaddr = (void*)((addr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t,*v_pml4e;
    uint64_t *pdpt,*v_pdpte,*pdpte;
    uint64_t *pdt,*v_pde,*pde;
    v_pml4t = KADDR_P2V(pml4t);
    v_pml4e = v_pml4t + GET_FIELD((addr_t)vaddr,ADDR_PML4T_INDEX);
    ASSERT(*v_pml4e & PG_P);

    pdpt = (uint64_t*)(*v_pml4e & (~0xfff));
    pdpte = pdpt + GET_FIELD((addr_t)vaddr,ADDR_PDPT_INDEX);
    v_pdpte = KADDR_P2V(pdpte);
    ASSERT(*v_pdpte & PG_P);

    pdt = (uint64_t*)(*v_pdpte & (~0xfff));
    pde = pdt + GET_FIELD((addr_t)vaddr,ADDR_PDT_INDEX);
    v_pde = KADDR_P2V(pde);
    ASSERT(*v_pde & PG_P);
    *v_pde = (*v_pde & ~0xfff) | flags;
    return;
}