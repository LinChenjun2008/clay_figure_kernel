/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#include <Efi.h>
#include <Guid/Acpi.h>
#include <Uefi/UefiAcpiDataTable.h>

#define __BOOTLOADER__
#include <common.h>
#undef __BOOTLOADER__

#include <config.h>

#define SIGNATURE_32(A,B,C,D) ( D << 24 | C << 16 | B << 8 | A)

#define MADT_SIGNATURE SIGNATURE_32('A','P','I','C') //"APIC"

#if 0
#define DISPLAY_INFO(msg) \
    do \
    { \
        gST->ConOut->SetAttribute(gST->ConOut,  0x0A | 0x00); \
        gST->ConOut->OutputString(gST->ConOut,L"[ INFO  ] "); \
        gST->ConOut->SetAttribute(gST->ConOut,  0x0F | 0x00); \
        gST->ConOut->OutputString(gST->ConOut,msg); \
        gST->ConOut->SetAttribute(gST->ConOut,  0x07 | 0x00); \
    }while(0);

#define DISPLAY_ERROR(msg) \
    do \
    { \
        gST->ConOut->SetAttribute(gST->ConOut,  0x0C | 0x00); \
        gST->ConOut->OutputString(gST->ConOut,L"[ ERROR ] "); \
        gST->ConOut->SetAttribute(gST->ConOut,  0x0F | 0x00); \
        gST->ConOut->OutputString(gST->ConOut,msg); \
    }while(0);

#endif

#define DISPLAY_INFO(msg) (void)0
#define DISPLAY_ERROR(msg) (void)0

EFI_SYSTEM_TABLE                *gST;
EFI_BOOT_SERVICES               *gBS;
EFI_GRAPHICS_OUTPUT_PROTOCOL    *Gop;
EFI_HANDLE                       gImageHandle;

EFI_GUID gEfiGraphicsOutputProtocolGuid   = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID gEfiFileInfoGuid                 = EFI_FILE_INFO_ID;
EFI_GUID gEfiAcpiTableGuid                = EFI_ACPI_TABLE_GUID;

EFI_STATUS SetVideoMode(int x,int y);
EFI_STATUS DisplayLogo();
EFI_STATUS ReadFile
(
    CHAR16 *FileName,
    EFI_PHYSICAL_ADDRESS *FileBufferBase,
    EFI_ALLOCATE_TYPE BufferType,
    UINT64 *FileSize
);
EFI_STATUS GetMemoryMap(memory_map_t *memmap);
VOID CreatePage(EFI_PHYSICAL_ADDRESS PML4T);

int CompareGuid(EFI_GUID *guid1,EFI_GUID *guid2)
{
    return    ((guid1->Data1 == guid2->Data1)
            && (guid1->Data2 == guid2->Data2)
            && (guid1->Data3 == guid2->Data3)
            && (guid1->Data4[0] == guid2->Data4[0])
            && (guid1->Data4[1] == guid2->Data4[1])
            && (guid1->Data4[2] == guid2->Data4[2])
            && (guid1->Data4[3] == guid2->Data4[3])
            && (guid1->Data4[4] == guid2->Data4[4])
            && (guid1->Data4[5] == guid2->Data4[5])
            && (guid1->Data4[6] == guid2->Data4[6])
            && (guid1->Data4[7] == guid2->Data4[7]));
}

EFI_STATUS
EFIAPI
UefiMain
(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status = EFI_SUCCESS;

    // init
    gST = SystemTable;
    gBS = SystemTable->BootServices;
    gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,NULL,(VOID**)&Gop);
    gImageHandle = ImageHandle;

    // disable watch dog timer
    gST->BootServices->SetWatchdogTimer(0,0,0,NULL);

    // Status = SetVideoMode(HORIZONTAL_RESOLUTION,VERTICAL_RESOLUTION);
    // if (EFI_ERROR(Status))
    // {
    //     DISPLAY_ERROR(L"Can not set video mode.\r\n");
    // }

    DisplayLogo();

    // gST->ConOut->SetAttribute(gST->ConOut,  0x0F | 0x00);
    DISPLAY_INFO(L"\r---------------------------------------------------------\r\n"
                    "----- Clay Figure Boot v1.0                         -----\r\n"
                    "----- Copyright (c) LinChenjun,All Rights Reserved. -----\r\n"
                    "---------------------------------------------------------\r\n");

    // prepare boot info
    boot_info_t *boot_info = (boot_info_t*)0x310000;
    Status = gBS->AllocatePages(AllocateAddress,EfiLoaderData,1,(EFI_PHYSICAL_ADDRESS*)&boot_info);
    if (EFI_ERROR(Status))
    {
        DISPLAY_ERROR(L"Can not allocate bootinfo address.\r\n");
    }
    gBS->SetMem(boot_info,sizeof(*boot_info),0);
    boot_info->magic = 0x5a42cb1613d4a62f;
    boot_info->graph_info.frame_buffer_base     = 0xffff807fc0000000;
    boot_info->graph_info.horizontal_resolution = Gop->Mode->
                                                  Info->HorizontalResolution;
    boot_info->graph_info.vertical_resolution   = Gop->Mode->
                                                  Info->VerticalResolution;
    boot_info->graph_info.pixel_per_scanline    = Gop->Mode->Info->PixelsPerScanLine;
    // load file
    DISPLAY_INFO(L"Load Files ...\r\n");
    uint8_t file_index;
    boot_info->loaded_files = 0;
    for (file_index = 0;
         file_index < sizeof(Files) / sizeof(Files[0]);
         file_index++)
    {
        DISPLAY_INFO(L"    Load file: ");
        // gST->ConOut->OutputString(gST->ConOut,Files[file_index].Name);
        // gST->ConOut->OutputString(gST->ConOut,L"\r\n");
        UINT64 FileSize = 0;
        boot_info->loaded_file[boot_info->loaded_files] = Files[file_index].Info;
        Status = ReadFile(Files[file_index].Name,
                          &boot_info->loaded_file[boot_info->loaded_files].base_address,
                          Files[file_index].FileBufferType,
                          &FileSize);
        if (EFI_ERROR(Status))
        {
            DISPLAY_ERROR(L"    Load File ERROR.\r\n");
            continue;
        }
        DISPLAY_INFO(L"    Load file success.\r\n");
        boot_info->loaded_file[boot_info->loaded_files].size         = FileSize;
        boot_info->loaded_files++;
    }
    DISPLAY_INFO(L"Load File Done.\r\n");

    // Get memory map
    DISPLAY_INFO(L"Get Memory map.\r\n");
    boot_info->memory_map.map_size = 4096 * 4,
    boot_info->memory_map.buffer = NULL,
    boot_info->memory_map.map_key = 0,
    boot_info->memory_map.descriptor_size = 0,
    boot_info->memory_map.descriptor_version = 0,
    Status = GetMemoryMap(&boot_info->memory_map);
    if (EFI_ERROR(Status))
    {
        DISPLAY_ERROR(L"Get Memory map Error. Boot Failed.\r\n");
        while (1) continue;
    }
    DISPLAY_INFO(L"Get Memory Map Success.\r\n");

    // Get MADT
    //    Get RSDP
    DISPLAY_INFO(L"Get MADT.\r\n");
    EFI_CONFIGURATION_TABLE *ConfigTable = gST->ConfigurationTable;
    EFI_ACPI_6_4_ROOT_SYSTEM_DESCRIPTION_POINTER *rsdp;
    UINTN i;
    for (i = 0;i < gST->NumberOfTableEntries;i++)
    {
        if (CompareGuid(&ConfigTable->VendorGuid,&gEfiAcpiTableGuid))
        {
            rsdp = ConfigTable->VendorTable;
            if (rsdp->Revision == 2)
            {
                DISPLAY_INFO(L"    Get RSDP Success.\r\n");
                break;
            }
        }
        ConfigTable++;
    }

    //    Get MADT
    DISPLAY_INFO(L"    Get MADT.\r\n");
    XSDT_TABLE *xsdt = (void*)rsdp->XsdtAddress;
    UINT32 entries = (xsdt->Header.Length - sizeof(xsdt->Header)) / 8;
    uint64_t *point_to_other_sdt = &xsdt->Entry;

    for (i = 0;i < entries;i++)
    {
        EFI_ACPI_DESCRIPTION_HEADER *h = \
                (EFI_ACPI_DESCRIPTION_HEADER*)point_to_other_sdt[i];
        if (h->Signature == MADT_SIGNATURE)
        {
            boot_info->madt_addr = (void*)((EFI_PHYSICAL_ADDRESS)boot_info
                                    + sizeof(boot_info_t));
            gBS->CopyMem(boot_info->madt_addr,h,h->Length);
            DISPLAY_INFO(L"Get MADT Success.\r\n");
            break;
        }
    }
    if (i == entries)
    {
        DISPLAY_ERROR(L"Get MADT Error. Boot failed.\r\n");
        while (1) continue;
    }

    // Create Page
    DISPLAY_INFO(L"Create Page.\r\n");
    UINTN PML4T_POS = 0x5f9000;
    Status = gBS->AllocatePages(AllocateAddress,EfiLoaderData,7,&PML4T_POS);
    if (EFI_ERROR(Status))
    {
        DISPLAY_ERROR(L"Allocate Page Failed.\r\n");
    }
    CreatePage(PML4T_POS);
    DISPLAY_INFO(L"Create Page Done.\r\n");

    DISPLAY_INFO(L"Exit BootService & go to kernel.\r\n");
    gBS->ExitBootServices(gImageHandle,boot_info->memory_map.map_key);
    UINT64 (*kernel_main)(void) = (void*)0x100000;
    kernel_main();
    while (1) continue;
    return Status;
}