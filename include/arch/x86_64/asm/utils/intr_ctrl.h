// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_UTILS_INTR_CTRL_H__
#define __ASM_UTILS_INTR_CTRL_H__

void ASMLINKAGE io_cli(void);
void ASMLINKAGE io_sti(void);
void ASMLINKAGE io_hlt(void);
void ASMLINKAGE io_stihlt(void);

#endif /* __ASM_UTILS_INTR_CTRL_H__ */
