// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_H__
#define __MEM_H__

void mem_init(struct system_info *system_info);

struct mm_struct *allocate_mm_struct(void);
int               copy_mm_struct(struct task *dst, struct task *src);
void              destory_mm_struct(struct mm_struct *mm);
void             *mm_allocate_address(void *addr, size_t pages);
void              mm_free_address(void *addr, size_t pages);
void              mm_map(struct task *task, phys_addr_t phys, uintptr_t virt);
void              mm_unmap(struct task *task, uintptr_t virt);
void        mm_map_cow(struct task *task, phys_addr_t phys, uintptr_t virt);
phys_addr_t mm_allocate_a_page(void);
void        mm_remove_a_page(phys_addr_t addr);
size_t      mm_free_a_page(phys_addr_t addr);

#endif