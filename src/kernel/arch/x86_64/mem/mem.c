#include <kernel/global.h>
#include <lib/bitmap.h>
#include <std/string.h>
#include <intr.h>
#include <mem/mem.h>
#include <device/spinlock.h>

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
    uint32_t  Type;
    phy_addr_t PhysicalStart;
    addr_t VirtualStart;
    uint64_t  NumberOfPages;
    uint64_t  Attribute;
} __attribute__((aligned(16))) EFI_MEMORY_DESCRIPTOR;

struct
{
    bitmap_t   page_bitmap;
    spinlock_t lock;
} mem;

PRIVATE uint8_t  page_bitmap_map[PAGE_BITMAP_BYTES_LEN];

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
        case EfiLoaderData: ///
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

PUBLIC void mem_init()
{
    mem.page_bitmap.map = page_bitmap_map;
    mem.page_bitmap.btmp_bytes_len = PAGE_BITMAP_BYTES_LEN;
    init_bitmap(&mem.page_bitmap);
    init_spinlock(&mem.lock);
    int number_of_memory_desct;
    number_of_memory_desct = g_boot_info->memory_map.map_size
                           / g_boot_info->memory_map.descriptor_size;
    size_t    total_free  = 0;
    size_t    mem_size    = 0;
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
        mem_size        += efi_memory_desc[i].NumberOfPages << 12;
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
            start = page_size_round_up(start);
            end   = page_size_round_down(end);
            if(end >= start)
            {
                total_free += (end - start) * PG_SIZE;
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
    pr_log("\1 Memory: total: %d MB ( %d GB ),free: %d MB (%d GB)\n",
            mem_size >> 20,mem_size >> 30,
            total_free >> 20,total_free >> 30);
    return;
}

PUBLIC status_t alloc_physical_page(IN(uint64_t number_of_pages),OUT(void *addr))
{
    if (addr == NULL)
    {
        return K_ERROR;
    }
    if (number_of_pages == 0)
    {
        return K_ERROR;
    }
    spinlock_lock(&mem.lock);
    uint32_t index;
    status_t status = bitmap_alloc(IN(&mem.page_bitmap,number_of_pages),OUT(&index));
    if (ERROR(status))
    {
        spinlock_unlock(&mem.lock);
        return K_ERROR;
    }
    phy_addr_t paddr = 0;

    uint64_t i;
    for (i = index;i < index + number_of_pages;i++)
    {
        bitmap_set(&mem.page_bitmap,i,1);
    }
    paddr = (0UL + (phy_addr_t)index * PG_SIZE);
    memset(KADDR_P2V(paddr),0,number_of_pages * PG_SIZE);

    spinlock_unlock(&mem.lock);
    *(phy_addr_t*)addr = paddr;
    return K_SUCCESS;
}

PUBLIC void free_physical_page(void *addr,uint64_t number_of_pages)
{
    if (number_of_pages == 0)
    {
        return;
    }
    if (addr == NULL || ((((phy_addr_t)addr) & 0x1fffff) != 0))
    {
        return;
    }
    spinlock_lock(&mem.lock);
    phy_addr_t i;
    for (i = (phy_addr_t)addr / PG_SIZE;
         i < (phy_addr_t)addr / PG_SIZE + number_of_pages;i++)
    {
        bitmap_set(&mem.page_bitmap,i,0);
    }
    spinlock_unlock(&mem.lock);
    return;
}


PUBLIC uint64_t* pml4t_entry(void *pml4t,void *vaddr)
{
    return (uint64_t*)pml4t + ADDR_PML4T_INDEX(vaddr);
}

PUBLIC uint64_t* pdpt_entry(void *pml4t,void *vaddr)
{
    return (uint64_t*)(*(uint64_t*)KADDR_P2V(pml4t_entry(pml4t,vaddr)) & ~0xfff)
            + ADDR_PDPT_INDEX(vaddr);
}

PUBLIC uint64_t* pdt_entry(void *pml4t,void *vaddr)
{
    return (uint64_t*)(*(uint64_t*)KADDR_P2V(pdpt_entry(pml4t,vaddr)) & ~0xfff)
            + ADDR_PDT_INDEX(vaddr);
}

PUBLIC void* to_physical_address(void *pml4t,void *vaddr)
{
    uint64_t *v_pml4t,*v_pml4e;
    uint64_t *pdpt,*v_pdpte,*pdpte;
    uint64_t *pdt,*v_pde,*pde;
    v_pml4t = KADDR_P2V(pml4t);
    v_pml4e = v_pml4t + ADDR_PML4T_INDEX(vaddr);
    if (!(*v_pml4e & PG_P))
    {
        pr_log("\3 %s:vaddr pml4e not exist: %p\n",__func__,vaddr);
        return NULL;
    }
    pdpt = (uint64_t*)(*v_pml4e & (~0xfff));
    pdpte = pdpt + ADDR_PDPT_INDEX(vaddr);
    v_pdpte = KADDR_P2V(pdpte);
    if (!(*v_pdpte & PG_P))
    {
        pr_log("\3 %s:vaddr pdpte not exist: %p\n",__func__,vaddr);
        return NULL;
    }
    pdt = (uint64_t*)(*v_pdpte & (~0xfff));
    pde = pdt + ADDR_PDT_INDEX(vaddr);
    v_pde = KADDR_P2V(pde);
    if (!(*v_pde & PG_P))
    {
        pr_log("\3 %s:vaddr pde not exist: %p\n",__func__,vaddr);
        return NULL;
    }
    return (void*)((*v_pde & ~0xfff) + ADDR_OFFSET(vaddr));
}

PUBLIC void page_map(uint64_t *pml4t,void *paddr,void *vaddr)
{
    paddr = (void*)((phy_addr_t)paddr & ~(PG_SIZE - 1));
    vaddr = (void*)(    (addr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t,*v_pml4e;
    uint64_t *v_pdpt,*pdpt,*v_pdpte,*pdpte;
    uint64_t *v_pdt,*pdt,*v_pde,*pde;
    v_pml4t = KADDR_P2V(pml4t);
    v_pml4e = v_pml4t + ADDR_PML4T_INDEX(vaddr);
    status_t status;
    if (!(*v_pml4e & PG_P))
    {
        status = pmalloc(IN(PT_SIZE),OUT(&pdpt));
        if (ERROR(status))
        {
            pr_log("\3 Can not alloc addr for pdpt.\n");
        }
        v_pdpt = KADDR_P2V(pdpt);
        memset(v_pdpt,0,PT_SIZE);
        *v_pml4e = (uint64_t)pdpt | PG_US_U | PG_RW_W | PG_P;
    }
    pdpt = (uint64_t*)(*v_pml4e & (~0xfff));
    pdpte = pdpt + ADDR_PDPT_INDEX(vaddr);
    v_pdpte = KADDR_P2V(pdpte);
    if (!(*v_pdpte & PG_P))
    {
        status = pmalloc(IN(PT_SIZE),OUT(&pdt));
        if (ERROR(status))
        {
            pr_log("\3 Can not alloc addr for pdt.\n");
        }
        v_pdt = KADDR_P2V(pdt);
        memset(v_pdt,0,PT_SIZE);
        *v_pdpte = (uint64_t)pdt | PG_US_U | PG_RW_W | PG_P;
    }
    pdt = (uint64_t*)(*v_pdpte & (~0xfff));
    pde = pdt + ADDR_PDT_INDEX(vaddr);
    v_pde = KADDR_P2V(pde);
    *v_pde = (uint64_t)paddr | PG_US_U | PG_RW_W | PG_P | PG_SIZE_2M;
}

PUBLIC void page_unmap(uint64_t *pml4t,void *vaddr)
{
    vaddr = (void*)((addr_t)vaddr & ~(PG_SIZE - 1));
    uint64_t *v_pml4t,*v_pml4e;
    uint64_t *pdpt,*v_pdpte,*pdpte;
    uint64_t *pdt,*v_pde,*pde;
    v_pml4t = KADDR_P2V(pml4t);
    v_pml4e = v_pml4t + ADDR_PML4T_INDEX(vaddr);
    if (!(*v_pml4e & PG_P))
    {
        pr_log("\3 vaddr pml4e not exist: %p\n",vaddr);
        return;
    }
    pdpt = (uint64_t*)(*v_pml4e & (~0xfff));
    pdpte = pdpt + ADDR_PDPT_INDEX(vaddr);
    v_pdpte = KADDR_P2V(pdpte);
    if (!(*v_pdpte & PG_P))
    {
        pr_log("\3 vaddr pdpte not exist: %p\n",vaddr);
        return;
    }
    pdt = (uint64_t*)(*v_pdpte & (~0xfff));
    pde = pdt + ADDR_PDT_INDEX(vaddr);
    v_pde = KADDR_P2V(pde);
    if (!(*v_pde & PG_P))
    {
        pr_log("\3 vaddr pde not exist: %p\n",vaddr);
        return;
    }
    *v_pde &= ~PG_P;
}