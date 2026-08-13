// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <bootloader.h>
#include <elf.h>

static int load_exec(
    efi_physical_address_t file,
    uintptr_t              relocate_base,
    uintptr_t             *entry
)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)file;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)ehdr + ehdr->e_phoff);

    uintptr_t addr_lo = 0xffffffffffffffff;
    uintptr_t addr_hi = 0;

    Elf64_Half i;
    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
        {
            continue;
        }

        if (addr_lo > phdr[i].p_vaddr)
        {
            addr_lo = phdr[i].p_vaddr;
        }
        if (addr_hi < phdr[i].p_vaddr + phdr[i].p_memsz)
        {
            addr_hi = phdr[i].p_vaddr + phdr[i].p_memsz;
        }
    }

    if (addr_hi - addr_lo > 0x2fffff)
    {
        return -1;
    }
    size_t pages = (addr_hi - addr_lo) / 0x1000 + 1;
    int    status;

    uintptr_t offset        = -relocate_base;
    uintptr_t physical_base = addr_lo + offset;

    status = boot_services->allocate_pages(
        EFI_ALLOCATE_ADDRESS, EFI_LOADER_DATA, pages, &physical_base
    );
    if (EFI_ERROR(status))
    {
        printf(L"load_exec: allocate_pages(): failed.\r\n");
        return status;
    }

    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
        {
            continue;
        }
        uintptr_t destination = phdr[i].p_vaddr + offset;
        uintptr_t source      = file + phdr[i].p_offset;
        size_t    mem_size    = phdr[i].p_memsz;
        size_t    file_size   = phdr[i].p_filesz;

        boot_services->set_mem((void *)destination, mem_size, 0);
        boot_services->copy_mem((void *)destination, (void *)source, file_size);
    }
    *entry = ehdr->e_entry;
    return 0;
}


static int load_dyn(
    efi_physical_address_t file,
    uintptr_t             *physical_base,
    uintptr_t             *relocate_base,
    uintptr_t             *entry
)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)file;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)ehdr + ehdr->e_phoff);

    uintptr_t addr_lo = 0xffffffffffffffff;
    uintptr_t addr_hi = 0;

    Elf64_Half dynamic_index = 0;
    Elf64_Half i;
    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_DYNAMIC)
        {
            dynamic_index = i;
            continue;
        }
        if (phdr[i].p_type != PT_LOAD)
        {
            continue;
        }
        if (addr_lo > phdr[i].p_vaddr)
        {
            addr_lo = phdr[i].p_vaddr;
        }
        if (addr_hi < phdr[i].p_vaddr + phdr[i].p_memsz)
        {
            addr_hi = phdr[i].p_vaddr + phdr[i].p_memsz;
        }
    }

    if (dynamic_index == 0)
    {
        printf(L"load_dyn: No dynamic.\r\n");
        return -1;
    }

    if (addr_hi - addr_lo > 0x2fffff)
    {
        printf(L"load_dyn: Too large.\r\n");
        return -2;
    }
    size_t pages = (addr_hi - addr_lo) / 0x1000 + 1;
    int    status;

    status = boot_services->allocate_pages(
        EFI_ALLOCATE_ADDRESS, EFI_LOADER_DATA, pages, physical_base
    );
    if (EFI_ERROR(status))
    {
        printf(L"load_dyn: allocate_pages(): failed.\r\n");
        return status;
    }

    uintptr_t offset          = *physical_base - addr_lo;
    uintptr_t relocate_offset = *relocate_base - addr_lo + *physical_base;

    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
        {
            continue;
        }
        uintptr_t destination = phdr[i].p_vaddr + offset;
        uintptr_t source      = file + phdr[i].p_offset;
        size_t    mem_size    = phdr[i].p_memsz;
        size_t    file_size   = phdr[i].p_filesz;

        boot_services->set_mem((void *)destination, mem_size, 0);
        boot_services->copy_mem((void *)destination, (void *)source, file_size);
    }

    // physical address
    *entry = ehdr->e_entry + offset;

    // dynamic linking
    Elf64_Phdr *dyn_phdr       = &phdr[dynamic_index];
    Elf64_Dyn  *dyn            = (Elf64_Dyn *)(dyn_phdr->p_vaddr + offset);
    Elf64_Rela *reloc          = NULL;
    size_t      relocate_count = 0;

    // char       *strtab        = NULL;
    // Elf64_sym_t  *symtab        = NULL;

    while (dyn->d_tag != DT_NULL)
    {
        switch (dyn->d_tag)
        {
            // case DT_STRTAB:
            //     strtab = (char *)(dyn->d_un.d_ptr + offset);
            //     break;
            // case DT_SYMTAB:
            //     symtab = (Elf64_sym_t *)(dyn->d_un.d_ptr + offset);
            //     break;
            case DT_RELA:
                reloc = (Elf64_Rela *)(dyn->d_un.d_ptr + offset);
                break;
            case DT_RELASZ:
                relocate_count = dyn->d_un.d_val / sizeof(Elf64_Rela);
                break;
            default:
                break;
        }
        dyn++;
    }

    // relocate
    for (i = 0; i < relocate_count; i++)
    {
        Elf64_Rela *rela = &reloc[i];
        uint32_t    type = ELF64_R_TYPE(rela->r_info);

        // uint32_t      sym_idx  = ELF64_R_SYM(rela->r_info);
        // Elf64_sym_t  *sym     = &symtab[sym_idx];
        // const char *sym_name = strtab + sym->st_name;

        uintptr_t *address;
        address = (uintptr_t *)(rela->r_offset + offset);

        // uintptr_t value = sym->st_value + relocate_offset;

        switch (type)
        {
            // case R_X86_64_GLOB_DAT:
            // case R_X86_64_JUMP_SLOT:
            //     *address = value;
            //     break;
            case R_X86_64_RELATIVE:
                *address = rela->r_addend + relocate_offset;
                break;
            default:
                return -3;
                break;
        }
    }

    return 0;
}

int load_segment(
    efi_physical_address_t file,
    uintptr_t             *physical_base,
    uintptr_t             *relocate_base,
    uintptr_t             *entry
)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)file;
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        printf(L"load_segment: elf magic check failed.\r\n");
        return -1;
    }
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        printf(L"load_segment: elf class check failed.\r\n");
        return -1;
    }
    if (ehdr->e_ident[EI_VERSION] != EV_CURRENT)
    {
        printf(L"load_segment: elf ident version check failed.\r\n");
        return -1;
    }
    if (ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV)
    {
        printf(L"load_segment: elf ABI check failed.\r\n");
        return -1;
    }
    if (ehdr->e_version != EV_CURRENT)
    {
        printf(L"load_segment: elf version check failed.\r\n");
        return -1;
    }
    int status = 0;
    if (ehdr->e_type == ET_EXEC)
    {
        printf(L"load_segment: Load exec.\r\n");
        status = load_exec(file, *relocate_base, entry);
        return status;
    }
    if (ehdr->e_type == ET_DYN)
    {
        printf(L"load_segment: Load dynamic.\r\n");
        status = load_dyn(file, physical_base, relocate_base, entry);
        return status;
    }
    printf(L"load_segment: Load failed.\r\n");

    return -1;
}