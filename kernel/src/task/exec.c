// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <elf.h>
#include <mem.h>
#include <print.h>
#include <ramfs.h>
#include <std/string.h>
#include <task.h>
#include <task/struct.h>

// TEST
#include <print.h>

void *load_segment(void *file)
{
    struct task      *task = get_current_task();
    struct vm_struct *vm   = &task->mm->vm_map;

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)file;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)ehdr + ehdr->e_phoff);

    uintptr_t addr_lo = -1UL;
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
        return NULL;
    }

    if (addr_hi - addr_lo > 0x2fffff)
    {
        return NULL;
    }
    size_t pages = (addr_hi - addr_lo + (PG_SIZE - 1)) & ~PG_SIZE;

    uintptr_t base_address;
    base_address = (uintptr_t)mm_allocate_address(vm, NULL, pages);

    uintptr_t offset = base_address - addr_lo;

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
        memset((void *)destination, mem_size, 0);
        memcpy((void *)destination, (void *)source, file_size);
    }

    // dynamic linking
    Elf64_Phdr *dyn_phdr       = &phdr[dynamic_index];
    Elf64_Dyn  *dyn            = (Elf64_Dyn *)(dyn_phdr->p_vaddr + offset);
    Elf64_Rela *reloc          = NULL;
    size_t      relocate_count = 0;

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

        // uintptr_t value = sym->st_value + offset;

        switch (type)
        {
            // case R_X86_64_GLOB_DAT:
            // case R_X86_64_JUMP_SLOT:
            //     *address = value;
            //     break;
            case R_X86_64_RELATIVE:
                *address = rela->r_addend + offset;
                break;
            default:
                return NULL;
                break;
        }
    }
    return (void *)(ehdr->e_entry + offset);
}