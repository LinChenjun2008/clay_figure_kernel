// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_UTILS_IO_H__
#define __ASM_UTILS_IO_H__

uint32_t ASMLINKAGE io_in8(uint32_t port);
uint32_t ASMLINKAGE io_in16(uint32_t port);
uint32_t ASMLINKAGE io_in32(uint32_t port);

void ASMLINKAGE io_out8(uint32_t port, uint32_t data);
void ASMLINKAGE io_out16(uint32_t port, uint32_t data);
void ASMLINKAGE io_out32(uint32_t port, uint32_t data);

#endif /* __ASM_UTILS_IO_H__ */
