// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSINFO_H__
#define __SYSINFO_H__

#include <types.h>

struct cpu;
struct page_mgr;
struct task_mgr;

struct graphic_info
{
    phys_addr_t frame_buffer_base;
    uint32_t    horizontal_resolution;
    uint32_t    vertical_resolution;
    uint32_t    pixel_per_scanline;
};

struct memory_map
{
    uint64_t map_size;
    void    *buffer;
    uint64_t map_key;
    uint64_t descriptor_size;
    uint32_t descriptor_version;
};

struct boot_info
{
    void  *initramfs;
    size_t initramfs_size;

    phys_addr_t pg_dir;
    uintptr_t   relocate_base;

    phys_addr_t stack_base;
    size_t      stack_pages;

    struct memory_map   memory_map;
    struct graphic_info graphic_info;

    uintptr_t **sdt_baseaddr_array;
    uint32_t    sdt_entries;
};

struct system_info
{
    struct boot_info *boot_info;
    struct page_mgr  *page_mgr;
    struct cpu       *cpu;
    struct task_mgr  *task_mgr;
};

struct task_mgr    *get_task_mgr(void);
uint8_t             get_current_cpu_id(void);
struct cpu         *get_cpu_struct(uint8_t cpu_id);
struct cpu         *get_curr_cpu_struct(void);
struct page_mgr    *get_page_mgr(void);
struct system_info *get_system_info(void);
void                set_cpu_struct(struct cpu *cpu);

#endif /* __SYSINFO_H__ */