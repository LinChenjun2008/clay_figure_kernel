// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

.text

.global asm_sqrt
.type asm_sqrt,@function
asm_sqrt:
    sqrtsd %xmm0,%xmm0
    ret
