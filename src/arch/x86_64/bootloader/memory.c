/*
   Copyright 2024 LinChenjun

   本文件是Clay Figure Kernel的一部分。
   修改和/或再分发遵循GNU GPL version 3 (or any later version)
  
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

VOID CreatePage(EFI_PHYSICAL_ADDRESS PML4T)
{
    /*
    * 系统内存分配:
    * 0x100000 - 0x2fffff ( 2MB) -内核
    * 0x300000 - 0x30ffff (64KB) -内核栈
    * 0x310000 - 0x5f8fff ( 2MB) -bootinfo
    * 0x5f9000 - 0x5fffff (28KB) -内核页表(部分)
    * 0x600000 - ...             -空闲内存
    映射:
    0x0000000000000000 - 0x00000000ffffffff ==> 0x0000000000000000 - 0x00000000ffffffff
    0x0000000000000000 - 0x00000000ffffffff ==> 0xffff800000000000 - 0xffff8000ffffffff
    0xffff807fc0000000 - 显存
    PML4E 0         PDPTE 3       PDE 511       offset
    0(1)000 0000 0 | 000 0000 11 | 11 1111 111 | 1 1111 1111 1111 1111 1111
    0(8)    0    0       0    f       f    f       f    f    f    f    f
       1000 0000 0 | 111 1111 11 | 00 0000 000 | 0 0000 0000 0000 0000 0000
       8    0    7       f    c       0    0       0    0    0    0    0
    */
    EFI_PHYSICAL_ADDRESS PDPT;
    EFI_PHYSICAL_ADDRESS PDT;
    EFI_PHYSICAL_ADDRESS FB_PDT; // 用于显存

    gBS->SetMem((void*)PML4T,7 * 0x1000,0);
    PDPT  = PML4T + 1 * 0x1000;

    *((UINTN*)(PML4T + 0x000)) = PDPT | PG_US_U | PG_RW_W | PG_P; // 0x00000...
    *((UINTN*)(PML4T + 0x800)) = PDPT | PG_US_U | PG_RW_W | PG_P; // 0xffff8...

    EFI_PHYSICAL_ADDRESS Addr = 0;
    UINTN i;
    for (i = 0;i <= 3;i++)
    {
        PDT = PML4T + (2 + i) * 0x1000;
        *((UINTN*)PDPT + i) = PDT | PG_US_U | PG_RW_W | PG_P;
        UINTN j;
        for (j = 0;j < 512;j++)
        {
            *((UINTN*)PDT + j) = Addr | PG_US_U | PG_RW_W | PG_P | PG_SIZE_2M;
            Addr += 0x200000;
        }
    }
    FB_PDT   = PDT + 0x1000;
    *((UINTN*)(PDPT + 0xff8)) = FB_PDT | PG_US_U | PG_RW_W | PG_P;
    Addr = Gop->Mode->FrameBufferBase;
    for(i = 0;i < (Gop->Mode->FrameBufferSize + 0x1fffff) / 0x200000;i++)
    {
        *((UINTN*)FB_PDT + i) =
                                   Addr | PG_US_U | PG_RW_W | PG_P | PG_SIZE_2M;
        Addr += 0x200000;
    }
    return;
}
