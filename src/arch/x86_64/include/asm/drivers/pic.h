// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_DRIVER_PIC_H__
#define __ASM_DRIVER_PIC_H__

#define IRQ_START 0x20
#define IRQ_CNT   0xe0

void pic_init(struct boot_info *boot_info);
void send_eoi(void);

#endif /* __ASM_DRIVER_PIC_H__ */