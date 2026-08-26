// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_TASK_FORK_H__
#define __ASM_TASK_FORK_H__

int copy_process(struct task *dst, struct task *src);

#endif /* __ASM_TASK_FORK_H__ */