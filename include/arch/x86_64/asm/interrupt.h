// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_INTERRUPT_H__
#define __ASM_INTERRUPT_H__

#define INTR_CNT 0x100

#include <asm/ptrace.h>

enum intr_status
{
    INTR_OFF = 0,
    INTR_ON,
    MAX_INTR_STATUS,
};

enum intr_status intr_get_status(void);
enum intr_status intr_enable(void);
enum intr_status intr_disable(void);
enum intr_status intr_set_status(enum intr_status status);

void intr_init(void);

#endif /* __ASM_INTERUPT_H__ */