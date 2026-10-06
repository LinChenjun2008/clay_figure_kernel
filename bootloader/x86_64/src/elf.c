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

static void *load_exec(void *file, uintptr_t *phys, uintptr_t *virt)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)file;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)ehdr + ehdr->e_phoff);

    // 内核虚拟基址 = PT_LOAD 中最低的 p_vaddr
    uintptr_t  link_base = -1ULL;
    Elf64_Half i;
    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD && link_base > phdr[i].p_vaddr)
        {
            link_base = phdr[i].p_vaddr;
        }
    }

    size_t pages = DIV_ROUND_UP(calculate_load_size(file), PG_SIZE);

    int status = boot_services->allocate_pages(
        EFI_ALLOCATE_ADDRESS, EFI_LOADER_DATA, pages, phys
    );
    if (EFI_ERROR(status))
    {
        printf(L"load_exec: allocate_pages(): failed.\r\n");
        return NULL;
    }

    intptr_t offset = (intptr_t)*phys - (intptr_t)link_base;

    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
        {
            continue;
        }
        uintptr_t destination = (uintptr_t)((intptr_t)phdr[i].p_vaddr + offset);
        uintptr_t source      = (uintptr_t)file + phdr[i].p_offset;
        size_t    mem_size    = phdr[i].p_memsz;
        size_t    file_size   = phdr[i].p_filesz;

        boot_services->set_mem((void *)destination, mem_size, 0);
        boot_services->copy_mem((void *)destination, (void *)source, file_size);
    }

    *virt = link_base;
    return (void *)((intptr_t)ehdr->e_entry + offset);
}

void *load_segment(void *file, uintptr_t *phys, uintptr_t *virt)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)file;
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        printf(L"load_segment: elf magic check failed.\r\n");
        return NULL;
    }
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        printf(L"load_segment: elf class check failed.\r\n");
        return NULL;
    }
    if (ehdr->e_ident[EI_VERSION] != EV_CURRENT)
    {
        printf(L"load_segment: elf ident version check failed.\r\n");
        return NULL;
    }
    if (ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV)
    {
        printf(L"load_segment: elf ABI check failed.\r\n");
        return NULL;
    }
    if (ehdr->e_version != EV_CURRENT)
    {
        printf(L"load_segment: elf version check failed.\r\n");
        return NULL;
    }
    if (ehdr->e_type != ET_EXEC)
    {
        printf(L"load_segment: Not exec file.\r\n");
        return NULL;
    }
    return load_exec(file, phys, virt);
}