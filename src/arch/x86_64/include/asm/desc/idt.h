// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __X86_IDT_H__
#define __X86_IDT_H__

#pragma pack(1)
typedef struct
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  attribute;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} gate_desc_t;
#pragma pack()

void idt_init(void);
void load_idt(void);

#endif /* __X86_IDT_H__ */