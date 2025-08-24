// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <bootloader.h>

static EFI_STATUS
ElfLoadExec(EFI_PHYSICAL_ADDRESS ElfFile, EFI_PHYSICAL_ADDRESS *Entry)
{
    Elf64_Ehdr *Ehdr = (Elf64_Ehdr *)ElfFile;
    Elf64_Phdr *Phdr = (Elf64_Phdr *)((uintptr_t)Ehdr + Ehdr->e_phoff);

    EFI_PHYSICAL_ADDRESS AddrLo = 0xffffffffffffffff;
    EFI_PHYSICAL_ADDRESS AddrHi = 0;

    Elf64_Half i;
    for (i = 0; i < Ehdr->e_phnum; i++)
    {
        if (Phdr[i].p_type == PT_LOAD)
        {
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
        Printf(L"PT_LOAD too large (%d).\n\r", AddrHi - AddrLo);
    }
    UINTN      Pages = (AddrHi - AddrLo) / 0x1000 + 1;
    EFI_STATUS Status;

    EFI_PHYSICAL_ADDRESS Offset       = -KERNEL_TEXT_BASE;
    EFI_PHYSICAL_ADDRESS PhysicalBase = AddrLo + Offset;

    Status = gBS->AllocatePages(
        AllocateAddress, EfiLoaderCode, Pages, &PhysicalBase
    );
    if (EFI_ERROR(Status))
    {
        Printf(L"Can not allocate address.\n\r");
        return Status;
    }

    for (i = 0; i < Ehdr->e_phnum; i++)
    {
        if (Phdr[i].p_type == PT_LOAD)
        {
            EFI_PHYSICAL_ADDRESS Destination = Phdr[i].p_vaddr + Offset;
            EFI_PHYSICAL_ADDRESS Source      = ElfFile + Phdr[i].p_offset;
            UINTN                MemSize     = Phdr[i].p_memsz;
            UINTN                FileSize    = Phdr[i].p_filesz;

            gBS->SetMem((VOID *)Destination, MemSize, 0);
            gBS->CopyMem((VOID *)Destination, (VOID *)Source, FileSize);
        }
    }
    *Entry = Ehdr->e_entry;
    return EFI_SUCCESS;
}

static EFI_STATUS ElfLoadDyn(
    EFI_PHYSICAL_ADDRESS  ElfFile,
    EFI_PHYSICAL_ADDRESS *PhysicalBase,
    EFI_VIRTUAL_ADDRESS  *RelocateBase,
    EFI_PHYSICAL_ADDRESS *Entry
)
{
    Elf64_Ehdr *Ehdr = (Elf64_Ehdr *)ElfFile;
    Elf64_Phdr *Phdr = (Elf64_Phdr *)((uintptr_t)Ehdr + Ehdr->e_phoff);

    EFI_PHYSICAL_ADDRESS AddrLo = 0xffffffffffffffff;
    EFI_PHYSICAL_ADDRESS AddrHi = 0;

    Elf64_Half DynamicIndex = 0;
    Elf64_Half i;
    for (i = 0; i < Ehdr->e_phnum; i++)
    {
        if (Phdr[i].p_type == PT_LOAD)
        {
            if (AddrLo > Phdr[i].p_vaddr)
            {
                AddrLo = Phdr[i].p_vaddr;
            }
            if (AddrHi < Phdr[i].p_vaddr + Phdr[i].p_memsz)
            {
                AddrHi = Phdr[i].p_vaddr;
            }
        }

        if (Phdr[i].p_type == PT_DYNAMIC)
        {
            DynamicIndex = i;
        }
    }

    if (DynamicIndex == 0)
    {
        Printf(L"PT_DYNAMIC not found.\n\r");
        return EFI_ERR;
    }

    if (AddrHi - AddrLo > 0x2fffff)
    {
        Printf(L"PT_LOAD too large (%d).\n\r", AddrHi - AddrLo);
    }
    UINTN      Pages = (AddrHi - AddrLo) / 0x1000 + 1;
    EFI_STATUS Status;

    Status =
        gBS->AllocatePages(AllocateAddress, EfiLoaderCode, Pages, PhysicalBase);
    if (EFI_ERROR(Status))
    {
        Printf(L"Can not allocate address: %p.\n\r", *PhysicalBase);
        return Status;
    }

    EFI_PHYSICAL_ADDRESS Offset         = *PhysicalBase - AddrLo;
    EFI_VIRTUAL_ADDRESS  RelocateOffset = *RelocateBase - AddrLo;

    for (i = 0; i < Ehdr->e_phnum; i++)
    {
        if (Phdr[i].p_type == PT_LOAD)
        {
            EFI_PHYSICAL_ADDRESS Destination = Phdr[i].p_vaddr + Offset;
            EFI_PHYSICAL_ADDRESS Source      = ElfFile + Phdr[i].p_offset;
            UINTN                MemSize     = Phdr[i].p_memsz;
            UINTN                FileSize    = Phdr[i].p_filesz;

            gBS->SetMem((VOID *)Destination, MemSize, 0);
            gBS->CopyMem((VOID *)Destination, (VOID *)Source, FileSize);
        }
    }

    // Physical address
    *Entry = Ehdr->e_entry + Offset;

    // Dynamic Linking
    Elf64_Phdr *DynPhdr       = &Phdr[DynamicIndex];
    Elf64_Dyn  *Dyn           = (Elf64_Dyn *)(DynPhdr->p_vaddr + Offset);
    Elf64_Rela *Reloc         = NULL;
    UINTN       RelocateCount = 0;

    // char       *Strtab        = NULL;
    // Elf64_Sym  *Symtab        = NULL;

    while (Dyn->d_tag != DT_NULL)
    {
        switch (Dyn->d_tag)
        {
            // case DT_STRTAB:
            //     Strtab = (char *)(Dyn->d_un.d_ptr + Offset);
            //     break;
            // case DT_SYMTAB:
            //     Symtab = (Elf64_Sym *)(Dyn->d_un.d_ptr + Offset);
            //     break;
            case DT_RELA:
                Reloc = (Elf64_Rela *)(Dyn->d_un.d_ptr + Offset);
                break;
            case DT_RELASZ:
                RelocateCount = Dyn->d_un.d_val / sizeof(Elf64_Rela);
                break;
            default:
                break;
        }
        Dyn++;
    }

    // Relocate
    for (i = 0; i < RelocateCount; i++)
    {
        Elf64_Rela *Rela = &Reloc[i];
        UINT32      Type = ELF64_R_TYPE(Rela->r_info);

        // UINT32      SymIdx  = ELF64_R_SYM(Rela->r_info);
        // Elf64_Sym  *Sym     = &Symtab[SymIdx];
        // const char *SymName = Strtab + Sym->st_name;

        EFI_PHYSICAL_ADDRESS *Address;
        Address = (EFI_PHYSICAL_ADDRESS *)(Rela->r_offset + Offset);

        // EFI_PHYSICAL_ADDRESS Value = Sym->st_value + RelocateOffset;

        switch (Type)
        {
            // case R_X86_64_GLOB_DAT:
            // case R_X86_64_JUMP_SLOT:
            //     *Address = Value;
            //     break;
            case R_X86_64_RELATIVE:
                *Address = Rela->r_addend + RelocateOffset;
                break;
            default:
                Printf(L"Bad Type: %#x.\n\r", Type);
                break;
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS LoadSegment(
    EFI_PHYSICAL_ADDRESS  ElfFile,
    EFI_PHYSICAL_ADDRESS *PhysicalBase,
    EFI_VIRTUAL_ADDRESS  *RelocateBase,
    EFI_PHYSICAL_ADDRESS *Entry
)
{
    Elf64_Ehdr *Ehdr = (Elf64_Ehdr *)ElfFile;

    // check elf file
    if (Ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        Ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        Ehdr->e_ident[EI_MAG2] != ELFMAG2 || Ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        Printf(L"ELF Magic check error.\n\r");
        return EFI_ERR;
    }
    if (Ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        Printf(L"ELF not a 64-bit object.\n\r");
        return EFI_ERR;
    }
    if (Ehdr->e_ident[EI_VERSION] != EV_CURRENT)
    {
        Printf(L"ELF version check error.\n\r");
        return EFI_ERR;
    }
    if (Ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV)
    {
        Printf(L"ELF not System V abi.\n\r");
        return EFI_ERR;
    }
    if (Ehdr->e_version != EV_CURRENT)
    {
        Printf(L"ELF version check error.\n\r");
        return EFI_ERR;
    }

    EFI_STATUS Status = EFI_SUCCESS;
    // Executable
    if (Ehdr->e_type == ET_EXEC)
    {
        *PhysicalBase = 0;
        *RelocateBase = 0;
        Status        = ElfLoadExec(ElfFile, Entry);
        if (EFI_ERROR(Status))
        {
            Printf(L"Load ET_EXEC failed.\n\r");
        }
        return Status;
    }
    if (Ehdr->e_type == ET_DYN)
    {
        Status = ElfLoadDyn(ElfFile, PhysicalBase, RelocateBase, Entry);
        if (EFI_ERROR(Status))
        {
            Printf(L"Load ET_DYN failed.\n\r");
        }
        return Status;
    }
    CHAR16 *file_type[6] = { L"ET_NONE", L"ET_REL",  L"ET_EXEC",
                             L"ET_DYN",  L"ET_CORE", L"ET_NUM" };
    Printf(L"File type not support ( %s ).\n\r", file_type[Ehdr->e_type]);
    return EFI_ERR;
}
