// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_TASK_H__
#define __ASM_TASK_H__

#include <task/struct.h>

// arch/task.c
void           arch_set_current_task(task_struct_t *task);
task_struct_t *arch_get_current_task(void);
uint8_t        arch_get_current_cpu_id(void);
void           arch_switch_to(task_context_t **curr, task_context_t **next);
void           arch_task_active(task_struct_t *task);

// arch/process.c
void switch_to_user(void *func, uint64_t kstack);

#endif /* __ASM_TASK_H__ */