// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_MP_H__
#define __ASM_MP_H__

#define AP_START 0x10000

#define AP_PAGE_TABLE_PTR 0x1000
#define AP_BOOT_FLAG      0x1008
#define AP_STACK          0x1010
#define AP_ENTRY          0x1018

#ifndef __ASSEMBLER__

void mp_init(boot_info_t *boot_info);
void mp_start(void *mp_entry);

#endif /* __ASSEMBLER__ */

#endif /* __ASM_MP_H__ */