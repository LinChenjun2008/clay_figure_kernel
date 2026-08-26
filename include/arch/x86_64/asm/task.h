// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_TASK_H__
#define __ASM_TASK_H__

void create_task_context(struct task *task, void *func, void *arg);

void ASMLINKAGE
arch_switch_to(struct task_context **curr, struct task_context **next);
void arch_task_active(struct task *task);

#endif /* __ASM_TASK_H__ */