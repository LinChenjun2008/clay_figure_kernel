// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSINFO_H__
#define __SYSINFO_H__

#include <mem/struct.h>
#include <task/struct.h>

struct task_mgr    *get_task_mgr(void);
uint8_t             get_current_cpu_id(void);
struct cpu         *get_cpu_struct(uint8_t cpu_id);
struct cpu         *get_curr_cpu_struct(void);
struct page_mgr    *get_page_mgr(void);
struct system_info *get_system_info(void);
void                set_cpu_struct(struct cpu *cpu);

#endif /* __SYSINFO_H__ */