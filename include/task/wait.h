// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TASK_WAIT_H__
#define __TASK_WAIT_H__

// waitpid
#define WNOHANG 1

pid_t task_waitpid(pid_t pid, int *status, int options);

#endif /* __TASK_WAIT_H__ */