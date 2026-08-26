// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_UTILS_DESC_LOAD_H__
#define __ASM_UTILS_DESC_LOAD_H__

void ASMLINKAGE arch_load_gdt(void *gdt_ptr, uint16_t code, uint16_t data);
void ASMLINKAGE arch_lidt(void *idt_ptr);
void ASMLINKAGE arch_ltr(uint64_t sel);

#endif /* __ASM_UTILS_DESC_LOAD_H__ */
