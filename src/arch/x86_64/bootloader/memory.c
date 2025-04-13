/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <Efi.h>

#define __BOOTLOADER__
#include <common.h>
#undef __BOOTLOADER__

#define PG_P 0x1
#define PG_RW_R 0x0
#define PG_RW_W 0x2
#define PG_US_S 0x0
#define PG_US_U 0x4
#define PG_SIZE_2M 0x80

EFI_STATUS GetMemoryMap(memory_map_t *memmap)
{
    EFI_STATUS Status = EFI_SUCCESS;
    Status = gBS->AllocatePool(EfiLoaderData,memmap->map_size,&memmap->buffer);
    if(EFI_ERROR(Status))
    {
        return Status;
    }
    Status = gBS->GetMemoryMap
    (
        &memmap->map_size,
        (EFI_MEMORY_DESCRIPTOR*)memmap->buffer,
        &memmap->map_key,
        &memmap->descriptor_size,
        &memmap->descriptor_version
    );
    return Status;
}

VOID CreatePage(EFI_PHYSICAL_ADDRESS PG_TABLE)
{
    /*
    * 系统内存分配:
    * 0x100000 - 0x3fffff ( 3MB) - 内核
    * 0x400000 - 0x40ffff (64KB) - 内核栈
    * 0x410000 - 0x50ffff ( 1MB) - bootinfo
    * 0x510000 - 0x58ffff (32KB) - 内核页表(部分)
    * 0x590000 - 0x5fffff ( 6KB) - 空闲
    * 0x600000 - ...             -空闲内存
    映射:
    0x0000000000000000 - 0x00000000ffffffff ==> 0x0000000000000000 - 0x00000000ffffffff
    0x0000000000000000 - 0x00000000ffffffff ==> 0xffff800000000000 - 0xffff8000ffffffff
    0xffffffff80000000 - kernel
    PML4E 0         PDPTE 3       PDE 511       offset
    0(1)000 0000 0 | 000 0000 11 | 11 1111 111 | 1 1111 1111 1111 1111 1111
    0(8)    0    0       0    f       f    f       f    f    f    f    f
    PML4E 511       PDPTE 510     PDE 2         offset
       1111 1111 1 | 111 1111 10 | 00 0000 010 | 0 0000 0000 0000 0000 0000
       f    f    f       f    8       0    4       0    0    0    0    0
    */
    EFI_PHYSICAL_ADDRESS PML4T;
    EFI_PHYSICAL_ADDRESS PDPT;
    EFI_PHYSICAL_ADDRESS PDT;

    gBS->SetMem((void*)PG_TABLE,8 * 0x1000,0);
    PML4T = PG_TABLE; // 0x1000-> PML4T
    PG_TABLE += 0x1000;
    /*
    * 进行以下映射:
    0x0000000000000000 - 0x00000000ffffffff ==> 0x0000000000000000 - 0x00000000ffffffff
    0x0000000000000000 - 0x00000000ffffffff ==> 0xffff800000000000 - 0xffff8000ffffffff
    */
    EFI_PHYSICAL_ADDRESS addr = 0;
    PDPT  = PG_TABLE;
    PG_TABLE += 0x1000;
    ((UINTN*)PML4T)[000] = PDPT | PG_US_U | PG_RW_W | PG_P; // 0x00000...
    ((UINTN*)PML4T)[256] = PDPT | PG_US_U | PG_RW_W | PG_P; // 0xffff8...
    UINTN pdpt_index,pdt_index;
    for (pdpt_index = 0;pdpt_index < 4;pdpt_index++)
    {
        PDT = PG_TABLE;
        PG_TABLE += 0x1000;
        ((UINTN*)PDPT)[pdpt_index] = PDT | PG_US_U | PG_RW_W | PG_P;
        for (pdt_index = 0;pdt_index < 512;pdt_index++)
        {
            UINTN PDT_entry = addr | PG_US_U | PG_RW_W | PG_P | PG_SIZE_2M;
            ((UINTN*)PDT)[pdt_index] = PDT_entry;
            addr += 0x200000;
        }
    }

    /*
    * 0 - 0x400000 ==> 0xffffffff80000000 - 0xffffffff80400000
    */
    addr = 0;
    PDPT = PG_TABLE;
    PG_TABLE += 0x1000;
    ((UINTN*)PML4T)[511] = PDPT | PG_US_U | PG_RW_W | PG_P; // kernel
    PDT = PG_TABLE;
    PG_TABLE += 0x1000;
    ((UINTN*)PDPT)[510] = PDT | PG_US_U | PG_RW_W | PG_P;
    for (pdt_index = 0;pdt_index < 2;pdt_index++)
    {
        UINTN PDT_entry = addr | PG_US_U | PG_RW_W | PG_P | PG_SIZE_2M;
        ((UINTN*)PDT)[pdt_index] = PDT_entry;
        addr += 0x200000;
    }
    return;
}
