// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <syscall.h>

.text

.global SIG_TRAMPOLINE_START
SIG_TRAMPOLINE_START:

    movq $NR_SRET,%rdi
    syscall

    // fill to 16 bytes
    nop
    nop
    nop
    nop
    nop
    nop
    nop

.global SIG_TRAMPOLINE_END
SIG_TRAMPOLINE_END:
