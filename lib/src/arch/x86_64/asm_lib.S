// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

.global syscall_0
.global syscall_1
.global syscall_2
.global syscall_3
.global syscall_4
.global syscall_5

.type syscall_0,@function
.type syscall_1,@function
.type syscall_2,@function
.type syscall_3,@function
.type syscall_4,@function
.type syscall_5,@function

syscall_0:
syscall_1:
syscall_2:
syscall_3:
syscall_4:
syscall_5:
    movq %rcx,%r10
    syscall
    retq
