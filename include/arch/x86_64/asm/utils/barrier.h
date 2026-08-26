// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_UTILS_BARRIER_H__
#define __ASM_UTILS_BARRIER_H__

void ASMLINKAGE io_lfence(void);
void ASMLINKAGE io_sfence(void);
void ASMLINKAGE io_mfence(void);

#define BARRIER() asm volatile("" ::: "memory");

#endif /* __ASM_UTILS_BARRIER_H__ */
