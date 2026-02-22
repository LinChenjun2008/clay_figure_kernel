// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __INIT_H__
#define __INIT_H__

int kernel_main(struct boot_info *boot_info);
int ap_kernel_main(uint64_t stack);

void init_all(struct boot_info *boot_info);
void ap_init(uint64_t stack);

#endif /* __INIT_H__ */