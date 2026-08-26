// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2026 Lin Chenjun
 */

#include <task/struct.h>

.text

.global arch_get_task_mgr
.type arch_get_task_mgr,@function
arch_get_task_mgr:
    swapgs
    movq %gs:CPU_TASK_MGR, %rax
    swapgs
    retq
