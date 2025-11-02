// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 Lin Chenjun
 */

#include <bootloader.h>

#define PG_P       0x1
#define PG_RW_R    0x0
#define PG_RW_W    0x2
#define PG_US_S    0x0
#define PG_US_U    0x4
#define PG_SIZE_2M 0x80

#define PT_SIZE 0x1000
#define PG_SIZE 0x1000

EFI_STATUS GetMemoryMap(memory_map_t *memmap)
{
    EFI_STATUS Status = EFI_SUCCESS;
    Status =
        gBS->AllocatePool(EfiLoaderData, memmap->map_size, &memmap->buffer);
    if (EFI_ERROR(Status))
    {
        return Status;
    }
    Status = gBS->GetMemoryMap(
        &memmap->map_size,
        (EFI_MEMORY_DESCRIPTOR *)memmap->buffer,
        &memmap->map_key,
        &memmap->descriptor_size,
        &memmap->descriptor_version
    );
    return Status;
}

STATIC EFI_PHYSICAL_ADDRESS GetPageTable(EFI_PHYSICAL_ADDRESS *PG_TABLE)
{
    EFI_PHYSICAL_ADDRESS Addr = *PG_TABLE;
    *PG_TABLE += PT_SIZE;
    return Addr;
}

VOID CreatePage(EFI_PHYSICAL_ADDRESS PG_TABLE)
{
    /*
    * 系统内存分配:
    * 0x0100000 - 0x03fffff  (  3MB) - 内核
    * 0x0400000 - 0x0400fff  (  4KB) - 内核栈
    * 0x0401000 - 0x040ffff  ( 60KB) - boot info
    * 0x0410000 - 0x140ffff  (  8MB) - 内核页表(部分)
    * 0x1410000 - 0x1ffffff          - 空闲内存
    * 0x2000000 - ...                - 可用空间(32MiB以上)
    映射:
        0x0000000000000000 - 0x00000000ffffffff
    ==> 0x0000000000000000 - 0x00000000ffffffff

        0x0000000000000000 - 0x00000000ffffffff
    ==> 0xffff800000000000 - 0xffff8000ffffffff

        0xffffffff80000000 - 0xffffffff803fffff
    ==> kernel

        0xffffffffc0000000 - ...
    ==> frame buffer

    PML4E 0         PDPTE 3       PDE 511       offset
    0(1)000 0000 0 | 000 0000 11 | 11 1111 111 | 1 1111 1111 1111 1111 1111
    0(8)    0    0       0    f       f    f       f    f    f    f    f
    PML4E 511       PDPTE 510     PDE 2         offset
       1111 1111 1 | 111 1111 10 | 00 0000 010 | 0 0000 0000 0000 0000 0000
       f    f    f       f    8       0    4       0    0    0    0    0
    PML4E 511       PDPTE 511     PDE 0         offset
       1111 1111 1 | 111 1111 11 | 00 0000 000 | 0 0000 0000 0000 0000 0000
       f    f    f       f    c       0    0       0    0    0    0    0
    */
    UINTN *PML4T, *PDPT, *PDT, *PT;

    UINTN PML4E, PDPTE, PDE, PTE;

    // Clear page table
    UINTN i;
    for (i = 0; i < 0x1000 * 0x1000 / sizeof(UINTN); i++)
    {
        *((UINTN *)PG_TABLE + i) = 0;
    }

    PML4T = (UINTN *)GetPageTable(&PG_TABLE);

    /*
    * 进行以下映射:
        0x0000000000000000 - 0x00000000ffffffff
    ==> 0x0000000000000000 - 0x00000000ffffffff

        0x0000000000000000 - 0x00000000ffffffff
    ==> 0xffff800000000000 - 0xffff8000ffffffff
    */
    EFI_PHYSICAL_ADDRESS Addr = 0;

    PDPT = (UINTN *)GetPageTable(&PG_TABLE);

    PML4E      = (UINTN)PDPT | PG_US_U | PG_RW_W | PG_P;
    PML4T[000] = PML4E; // 0x00000...
    PML4T[256] = PML4E; // 0xffff8...

    UINTN pdpt_index, pdt_index, pt_index;
    for (pdpt_index = 0; pdpt_index < 4; pdpt_index++)
    {
        PDT              = (UINTN *)GetPageTable(&PG_TABLE);
        PDPTE            = (UINTN)PDT | PG_US_U | PG_RW_W | PG_P;
        PDPT[pdpt_index] = PDPTE;
        for (pdt_index = 0; pdt_index < 512; pdt_index++)
        {
            PT             = (UINTN *)GetPageTable(&PG_TABLE);
            PDE            = (UINTN)PT | PG_US_U | PG_RW_W | PG_P;
            PDT[pdt_index] = PDE;
            for (pt_index = 0; pt_index < 512; pt_index++)
            {
                PTE          = Addr | PG_US_U | PG_RW_W | PG_P;
                PT[pt_index] = PTE;
                Addr += PG_SIZE;
            }
        }
    }


    /*
    * 0 - 0x400000 ==> 0xffffffff80000000 - 0xffffffff80400000
    */
    Addr       = 0;
    PDPT       = (UINTN *)GetPageTable(&PG_TABLE);
    PML4E      = (UINTN)PDPT | PG_US_U | PG_RW_W | PG_P;
    PML4T[511] = PML4E; // kernel

    PDT       = (UINTN *)GetPageTable(&PG_TABLE);
    PDPTE     = (UINTN)PDT | PG_US_U | PG_RW_W | PG_P;
    PDPT[510] = PDPTE;
    for (pdt_index = 0; pdt_index < 2; pdt_index++)
    {
        PT             = (UINTN *)GetPageTable(&PG_TABLE);
        PDE            = (UINTN)PT | PG_US_U | PG_RW_W | PG_P;
        PDT[pdt_index] = PDE;
        for (pt_index = 0; pt_index < 512; pt_index++)
        {
            PTE          = Addr | PG_US_U | PG_RW_W | PG_P;
            PT[pt_index] = PTE;
            Addr += PG_SIZE;
        }
    }

    // Frame buffer
    Addr                = Gop->Mode->FrameBufferBase;
    PDT                 = (UINTN *)GetPageTable(&PG_TABLE);
    PDPTE               = (UINTN)PDT | PG_US_U | PG_RW_W | PG_P;
    PDPT[511]           = PDPTE;
    UINTN max_pdt_index = (Gop->Mode->FrameBufferSize + 0x1fffff) / 0x200000;
    for (pdt_index = 0; pdt_index < max_pdt_index; pdt_index++)
    {
        PT             = (UINTN *)GetPageTable(&PG_TABLE);
        PDE            = (UINTN)PT | PG_US_U | PG_RW_W | PG_P;
        PDT[pdt_index] = PDE;
        for (pt_index = 0; pt_index < 512; pt_index++)
        {
            PTE          = Addr | PG_US_U | PG_RW_W | PG_P;
            PT[pt_index] = PTE;
            Addr += PG_SIZE;
        }
    }
    return;
}
