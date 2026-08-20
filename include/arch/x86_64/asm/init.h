// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_INIT_H__
#define __ASM_INIT_H__

#include <base.h>

void init_all(struct system_info *system_info);
void ap_init_all(struct system_info *system_info, uintptr_t stack);

#endif /* __ASM_INIT_H__ */