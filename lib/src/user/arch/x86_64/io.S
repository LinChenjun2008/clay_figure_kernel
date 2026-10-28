// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2026 Lin Chenjun
 */

.text

.global io_in8
.type io_in8,@function
io_in8:
    movw  %di, %dx
    inb (%dx), %al
    retq

.global io_in16
.type io_in16,@function
io_in16:
    movw  %di, %dx
    inw (%dx), %ax
    retq

.global io_in32
.type io_in32,@function
io_in32:
    movw  %di, %dx
    inl (%dx), %eax
    retq

.global io_out8
.type io_out8,@function
io_out8:
    movw %di, %dx
    movw %si, %ax
    outb %al, (%dx)
    retq

.global io_out16
.type io_out16,@function
io_out16:
    movw %di, %dx
    movw %si, %ax
    outw %ax, (%dx)
    retq

.global io_out32
.type io_out32,@function
io_out32:
    movw  %di, %dx
    movl %esi, %eax
    outl %eax, (%dx)
    retq
