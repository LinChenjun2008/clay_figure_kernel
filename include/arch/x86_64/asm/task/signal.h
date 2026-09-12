// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_TASK_SIGNAL_H__
#define __ASM_TASK_SIGNAL_H__

#include <asm/ptrace.h>

void signal_check(struct pt_regs *regs);
int  sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);
word_t sys_sigret(struct pt_regs *regs);

#endif /* __ASM_TASK_SIGNAL_H__ */