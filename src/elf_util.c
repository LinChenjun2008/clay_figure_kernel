// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <elf.h>
#include <elf_util.h>

#ifndef ELF_ALLOCATE_PAGES
#    define ELF_ALLOCATE_PAGES(PAGES, ADDRESS) \
        (((uintptr_t)ADDRESS + pages) == 0)
#endif

#ifndef ELF_MEMSET
#    include <std/string.h>
#    define ELF_MEMSET(DEST, VALUE, SIZE) memset(DEST, VALUE, SIZE)
#endif

#ifndef ELF_MEMCPY
#    include <std/string.h>
#    define ELF_MEMCPY(DEST, SRC, SIZE) memcpy(DEST, SRC, SIZE)
#endif

#ifndef ELF_ERROR
#    define ELF_ERROR(STATUS) ((STATUS) < 0)
#endif

static int
elf_load_exec(uintptr_t elf_file, uintptr_t relocate_base, uintptr_t *entry)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elf_file;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)ehdr + ehdr->e_phoff);

    uintptr_t addr_lo = 0xffffffffffffffff;
    uintptr_t addr_hi = 0;

    Elf64_Half i;
    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            if (addr_lo > phdr[i].p_vaddr)
            {
                addr_lo = phdr[i].p_vaddr;
            }
            if (addr_hi < phdr[i].p_vaddr + phdr[i].p_memsz)
            {
                addr_hi = phdr[i].p_vaddr + phdr[i].p_memsz;
            }
        }
    }

    if (addr_hi - addr_lo > 0x2fffff)
    {
        return -ERROR;
    }
    size_t pages = (addr_hi - addr_lo) / 0x1000 + 1;
    int    status;

    uintptr_t offset        = -relocate_base;
    uintptr_t physical_base = addr_lo + offset;

    status = ELF_ALLOCATE_PAGES(pages, &physical_base);
    if (ELF_ERROR(status))
    {
        return status;
    }

    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            uintptr_t destination = phdr[i].p_vaddr + offset;
            uintptr_t source      = elf_file + phdr[i].p_offset;
            size_t    mem_size    = phdr[i].p_memsz;
            size_t    file_size   = phdr[i].p_filesz;

            ELF_MEMSET((void *)destination, 0, mem_size);
            ELF_MEMCPY((void *)destination, (void *)source, file_size);
        }
    }
    *entry = ehdr->e_entry;
    return 0;
}

static int elf_load_dyn(
    uintptr_t  elf_file,
    uintptr_t *physical_base,
    uintptr_t *relocate_base,
    uintptr_t *entry
)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elf_file;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)ehdr + ehdr->e_phoff);

    uintptr_t addr_lo = 0xffffffffffffffff;
    uintptr_t addr_hi = 0;

    Elf64_Half dynamic_index = 0;
    Elf64_Half i;
    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            if (addr_lo > phdr[i].p_vaddr)
            {
                addr_lo = phdr[i].p_vaddr;
            }
            if (addr_hi < phdr[i].p_vaddr + phdr[i].p_memsz)
            {
                addr_hi = phdr[i].p_vaddr + phdr[i].p_memsz;
            }
        }

        if (phdr[i].p_type == PT_DYNAMIC)
        {
            dynamic_index = i;
        }
    }

    if (dynamic_index == 0)
    {
        return -1;
    }

    if (addr_hi - addr_lo > 0x2fffff)
    {
        return -2;
    }
    size_t pages = (addr_hi - addr_lo) / 0x1000 + 1;
    int    status;

    status = ELF_ALLOCATE_PAGES(pages, physical_base);
    if (ELF_ERROR(status))
    {
        return status;
    }

    uintptr_t offset          = *physical_base - addr_lo;
    uintptr_t relocate_offset = *relocate_base - addr_lo + *physical_base;

    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            uintptr_t destination = phdr[i].p_vaddr + offset;
            uintptr_t source      = elf_file + phdr[i].p_offset;
            size_t    mem_size    = phdr[i].p_memsz;
            size_t    file_size   = phdr[i].p_filesz;

            ELF_MEMSET((void *)destination, 0, mem_size);
            ELF_MEMCPY((void *)destination, (void *)source, file_size);
        }
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
    uintptr_t  elf_file,
    uintptr_t *physical_base,
    uintptr_t *relocate_base,
    uintptr_t *entry
)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elf_file;

    // check elf file
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        return -ERROR;
    }
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        return -ERROR;
    }
    if (ehdr->e_ident[EI_VERSION] != EV_CURRENT)
    {
        return -ERROR;
    }
    if (ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV)
    {
        return -ERROR;
    }
    if (ehdr->e_version != EV_CURRENT)
    {
        return -ERROR;
    }

    int status = 0;
    // executable
    if (ehdr->e_type == ET_EXEC)
    {
        *physical_base = 0;
        status         = elf_load_exec(elf_file, *relocate_base, entry);
        return status;
    }
    if (ehdr->e_type == ET_DYN)
    {
        status = elf_load_dyn(elf_file, physical_base, relocate_base, entry);
        return status;
    }
    return -ERROR;
}
