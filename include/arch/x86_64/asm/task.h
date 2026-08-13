// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_TASK_H__
#define __ASM_TASK_H__

#include <task/struct.h>

uint8_t arch_get_current_cpu_id(void);
void    arch_set_cpu_struct(void);

void create_task_context(struct task *task, void *func, void *arg);

void arch_switch_to(struct task_context **curr, struct task_context **next);
void arch_task_active(struct task *task);

// process.c

void switch_to_user(void *func);

#endif /* __ASM_TASK_H__ */