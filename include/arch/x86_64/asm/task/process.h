// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_TASK_PROCESS_H__
#define __ASM_TASK_PROCESS_H__

#define USER_STACK_VADDR_TOP 0x0000800000000000
#define USER_VADDR_START     0x800000

void *create_pg_dir(void);
void  switch_to_user(void *func, void *arg);

#endif /* __ASM_TASK_PROCESS_H__ */