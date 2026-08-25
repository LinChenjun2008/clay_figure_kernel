// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

.text

.global arch_flush_tlb
.type arch_flush_tlb,@function
arch_flush_tlb:
    invlpg (%rdi)
    retq