// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_H__
#define __MEM_H__

struct mm_struct;
struct system_info;
struct task;

void mem_init(struct system_info *system_info);

struct mm_struct *allocate_mm_struct(void);
int               copy_mm_struct(struct task *dst, struct task *src);
void              destroy_mm_struct(struct mm_struct *mm);

#define ADDR_ALLOCATED 0
#define ADDR_MAPPED    1

int       mm_check_addr(struct task *task, void *addr, size_t size, int flags);
uintptr_t mm_allocate_address(uintptr_t addr, size_t pages);
int       mm_map(struct task *task, phys_addr_t phys, uintptr_t virt);
int       mm_unmap(struct task *task, uintptr_t virt);
int       mm_map_cow(struct task *task, phys_addr_t phys, uintptr_t virt);
void      mm_free_address(uintptr_t addr, size_t pages);
phys_addr_t mm_allocate_a_page(struct task *task);
void        mm_remove_a_page(struct task *task, phys_addr_t addr);
size_t      mm_free_a_page(struct task *task, phys_addr_t addr);

// 共享内存(IPC SHM)相关的地址空间操作
int mm_reserve_shared_range(struct task *task, uintptr_t virt, size_t pages);
uintptr_t mm_allocate_high(struct task *task, size_t pages);
int  mm_map_shared(struct task *task, phys_addr_t phys, uintptr_t virt, int ro);
void mm_detach_shared(struct task *task, uintptr_t virt, size_t pages);

#endif
