// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 Lin Chenjun
 */

#ifndef __BOOTLOADER_CONFIG_H__
#define __BOOTLOADER_CONFIG_H__

#include <Efi.h>
#include <Guid/Acpi.h>
#include <Procotol/LoadedImage.h>
#include <Uefi/UefiAcpiDataTable.h>
#include <common.h>
#include <elf.h>
#include <std/stdarg.h>

struct Files
{
    CHAR16           *Name;
    EFI_ALLOCATE_TYPE FileBufferType;
    file_info_t       Info;
};

#define KERNEL_NAME    L"Kernel\\clfgkrnl.sys"
#define INITRAMFS_NAME L"Kernel\\initramfs.img"

// main.c
int CompareGuid(EFI_GUID *guid1, EFI_GUID *guid2);

EFI_STATUS
EFIAPI
UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);

// video.c
EFI_STATUS SetVideoMode(UINT32 xsize, UINT32 ysize);
EFI_STATUS DisplayLogo(void);

// file.c
EFI_STATUS ReadFile(
    CHAR16               *FileName,
    EFI_PHYSICAL_ADDRESS *FileBufferBase,
    UINT64               *FileSize
);

// memory.c
EFI_STATUS GetMemoryMap(memory_map_t *memmap);
VOID       CreatePage(EFI_PHYSICAL_ADDRESS PML4T);

// elf.c
EFI_STATUS LoadSegment(
    EFI_PHYSICAL_ADDRESS  ElfFile,
    EFI_PHYSICAL_ADDRESS *PhysicalBase,
    EFI_VIRTUAL_ADDRESS  *RelocateBase,
    EFI_PHYSICAL_ADDRESS *Entry
);

// print.c
int Vsprintf(CHAR16 *buf, const CHAR16 *fmt, va_list ap);
int Sprintf(CHAR16 *buf, const CHAR16 *fmt, ...);
int Printf(const CHAR16 *fmt, ...);

#endif