// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <bootloader.h>
#include <elf.h>

size_t calculate_load_size(void *file)
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
    if (addr_hi < addr_lo)
    {
        return 0;
    }
    return addr_hi - addr_lo;
}

static int load_dyn(
    void      *file,
    uintptr_t *physical_base,
    uintptr_t *relocate_base,
    uintptr_t *entry
)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)file;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)ehdr + ehdr->e_phoff);

    uintptr_t addr_lo = -1ULL;

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
    }

    if (dynamic_index == 0)
    {
        printf(L"load_dyn: No dynamic.\r\n");
        return -1;
    }

    size_t pages = DIV_ROUND_UP(calculate_load_size(file), PG_SIZE);
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
    off_t     relocate_offset = *relocate_base - addr_lo;

    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
        {
            continue;
        }
        uintptr_t destination = phdr[i].p_vaddr + offset;
        uintptr_t source      = (uintptr_t)file + phdr[i].p_offset;
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

    while (dyn->d_tag != DT_NULL)
    {
        switch (dyn->d_tag)
        {
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

        uintptr_t *address;
        address = (uintptr_t *)(rela->r_offset + offset);

        switch (type)
        {
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
    void      *file,
    uintptr_t *physical_base,
    uintptr_t *relocate_base,
    uintptr_t *entry
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
        // printf(L"load_segment: Load exec.\r\n");
        // status = load_exec(file, *relocate_base, entry);
        // return status;
        printf(L"load_segment: Not dynamic file.\r\n");
        return -1;
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