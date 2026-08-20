// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_SYSINFO_H__
#define __ASM_SYSINFO_H__

struct task_mgr *arch_get_task_mgr(void);
uint8_t          arch_get_current_cpu_id(void);
void             arch_set_cpu_struct(struct cpu *cpu);

#endif /* __ASM_SYSINFO_H__ */