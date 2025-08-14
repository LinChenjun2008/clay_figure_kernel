// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <Efi.h>
#define __BOOTLOADER__
#include <common.h>
#undef __BOOTLOADER__

#include <elf.h>

EFI_STATUS
LoadSegment(
    EFI_PHYSICAL_ADDRESS  ElfFile,
    EFI_PHYSICAL_ADDRESS  RelocateBase,
    EFI_PHYSICAL_ADDRESS *Entry
)
{
    Elf64_Ehdr *Ehdr = (Elf64_Ehdr *)ElfFile;
    Elf64_Phdr *Phdr = (Elf64_Phdr *)((uintptr_t)Ehdr + Ehdr->e_phoff);

    // check elf file
    if (Ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        Ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        Ehdr->e_ident[EI_MAG2] != ELFMAG2 || Ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        gST->ConOut->OutputString(gST->ConOut, L"ELF Magic check error.\n\r");
        return EFI_ERR;
    }
    if (Ehdr->e_version != EV_CURRENT)
    {
        gST->ConOut->OutputString(gST->ConOut, L"ELF version check error.\n\r");
        return EFI_ERR;
    }

    // Executable
    if (Ehdr->e_type != ET_EXEC)
    {
        CHAR16 *file_type[6] = { L"ET_NONE", L"ET_REL",  L"ET_EXEC",
                                 L"ET_DYN",  L"ET_CORE", L"ET_NUM" };
        gST->ConOut->OutputString(gST->ConOut, L"File not Executable ( ");
        gST->ConOut->OutputString(gST->ConOut, file_type[Ehdr->e_type]);
        gST->ConOut->OutputString(gST->ConOut, L" )\n\r");
        return EFI_ERR;
    }
    EFI_PHYSICAL_ADDRESS AddrLo = 0xffffffffffffffff;
    EFI_PHYSICAL_ADDRESS AddrHi = 0;

    Elf64_Half i;
    for (i = 0; i < Ehdr->e_phnum; i++)
    {
        if (Phdr[i].p_type == PT_LOAD)
        {
            if (Phdr[i].p_vaddr == 0)
            {
                continue;
            }
            if (AddrLo > Phdr[i].p_vaddr)
            {
                AddrLo = Phdr[i].p_vaddr;
            }
            if (AddrHi < Phdr[i].p_vaddr + Phdr[i].p_memsz)
            {
                AddrHi = Phdr[i].p_vaddr;
            }
        }
    }
    if (AddrHi - AddrLo > 0x2fffff)
    {
        gST->ConOut->OutputString(gST->ConOut, L"PT_LOAD too large.\n\r");
    }
    UINTN      Pages = (AddrHi - AddrLo) / 0x1000 + 1;
    EFI_STATUS Status;
    if (RelocateBase == 0)
    {
        Status = gBS->AllocatePages(
            AllocateAnyPages, EfiLoaderCode, Pages, &RelocateBase
        );
        if (EFI_ERROR(Status))
        {
            gST->ConOut->OutputString(
                gST->ConOut, L"Can not allocate memory.\n\r"
            );
            return Status;
        }
    }
    else
    {
        Status = gBS->AllocatePages(
            AllocateAddress, EfiLoaderCode, Pages, &RelocateBase
        );
        if (EFI_ERROR(Status))
        {
            gST->ConOut->OutputString(
                gST->ConOut, L"Can not allocate address.\n\r"
            );
            return Status;
        }
    }
    gBS->SetMem((VOID *)RelocateBase, Pages * 0x1000, 0);
    EFI_PHYSICAL_ADDRESS RelocateOffset = 0;
    for (i = 0; i < Ehdr->e_phnum; i++)
    {
        if (Phdr[i].p_type == PT_LOAD)
        {
            if (Phdr[i].p_vaddr == 0)
            {
                continue;
            }
            RelocateOffset = Phdr[i].p_vaddr - AddrLo;
            gBS->SetMem(
                (VOID *)(RelocateBase + RelocateOffset), Phdr->p_memsz, 0
            );
            gBS->CopyMem(
                (VOID *)(RelocateBase + RelocateOffset),
                (VOID *)(ElfFile + Phdr[i].p_offset),
                Phdr[i].p_filesz
            );
        }
    }
    *Entry = Ehdr->e_entry;
    return EFI_SUCCESS;
}