// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_TASK_H__
#define __ASM_TASK_H__

#include <task/struct.h>

void create_task_context(struct task *task, void *func, void *arg);

void arch_switch_to(struct task *curr, struct task *next);
void arch_task_active(struct task *task);

// process.c

void switch_to_user(void *func, void *arg);

#endif /* __ASM_TASK_H__ */