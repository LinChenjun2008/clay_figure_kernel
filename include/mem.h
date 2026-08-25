// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __MEM_H__
#define __MEM_H__

#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANON      0x20
#define MAP_ANONYMOUS MAP_ANON

void mem_init(struct system_info *system_info);

struct mm_struct *allocate_mm_struct(void);
void              destory_mm_struct(struct mm_struct *mm);
void             *mm_allocate_address(void *addr, size_t pages);
void              mm_free_address(void *addr, size_t pages);
void              mm_map(struct task *task, void *phys, void *virt);
void             *mm_allocate_a_page(void);

#endif