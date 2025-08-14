// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <elf.h>
#include <mem/page.h> // PG_SIZE
#include <ramfs.h>    // ramfs_file_t
#include <read_elf.h>
#include <std/string.h> // memcpy

PUBLIC int check_elf(void *data)
{
    Elf64_Ehdr *ehdr = data;
    if (!memcpy(ehdr->e_ident, ELFMAG, SELFMAG))
    {
        return 0;
    }
    if (ehdr->e_version != EV_CURRENT)
    {
        return 0;
    }
    // Position-Independent Executable
    if (ehdr->e_type != ET_DYN)
    {
        return 0;
    }
    return 1;
}

PUBLIC void elf_pt_load_addr(ramfs_file_t *fp, void *addr_lo, void *addr_hi)
{
    Elf64_Ehdr *ehdr = fp->data;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)fp->data + ehdr->e_phoff);
    Elf64_Addr  p_lo = 0xffffffffffffffff, p_hi = 0;
    Elf64_Addr  p_vaddr_start, p_vaddr_end;
    Elf64_Half  i;
    for (i = 0; i < ehdr->e_phnum; i++)
    {
        p_vaddr_start = phdr[i].p_vaddr;
        p_vaddr_end   = p_vaddr_start + phdr[i].p_memsz;
        if (phdr[i].p_type == PT_LOAD)
        {
            if (p_lo > p_vaddr_start)
            {
                p_lo = p_vaddr_start;
            }
            if (p_hi < p_vaddr_end)
            {
                p_hi = p_vaddr_end;
            }
        }
    }
    *(Elf64_Addr *)addr_lo = p_lo;
    *(Elf64_Addr *)addr_hi = p_hi;
    return;
}

PUBLIC size_t elf_load_size(ramfs_file_t *fp)
{
    Elf64_Ehdr *ehdr = fp->data;
    if (!check_elf(ehdr))
    {
        PR_LOG(LOG_WARN, "Elf magic check failed.\n");
    }
    // 计算PT_LOAD段的总大小(addr_hi - addr_lo)
    Elf64_Addr addr_lo, addr_hi;
    elf_pt_load_addr(fp, &addr_lo, &addr_hi);
    return addr_hi - addr_lo;
}

PUBLIC
void elf_load_segment(ramfs_file_t *fp, uintptr_t addr)
{
    Elf64_Ehdr *ehdr = fp->data;
    if (!check_elf(ehdr))
    {
        PR_LOG(LOG_WARN, "Elf magic check failed.\n");
    }
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)fp->data + ehdr->e_phoff);
    Elf64_Addr  addr_lo, addr_hi;
    elf_pt_load_addr(fp, &addr_lo, &addr_hi);

    size_t    fsize         = 0; // 段在文件中的大小
    size_t    msize         = 0; // 段在内存中的大小
    intptr_t  offset        = 0; // 段在addr处的偏移地址
    uintptr_t relocate_addr = 0; // 重定位后的地址
    uintptr_t source_addr   = 0; // 原始地址(文件)

    Elf64_Half i;
    for (i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            offset = phdr[i].p_vaddr - addr_lo;

            relocate_addr = addr + offset;
            source_addr   = (uintptr_t)fp->data + phdr[i].p_offset;
            fsize         = phdr[i].p_filesz;
            msize         = phdr[i].p_memsz;
            // clear memory
            memset((void *)relocate_addr, 0, msize);
            memcpy((void *)relocate_addr, (void *)source_addr, fsize);
        }
    }
}