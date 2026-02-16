// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

.text

.global kernel_entry
kernel_entry:
    movq 16(%rdi),%rax          // page_table_pos
    movq %rax,%cr3
    movq %rsi,%rsp              // kernel stack
    leaq kernel_main(%rip),%rax // kernel_entry
    addq 24(%rdi),%rax          // relocate_base
    jmpq *%rax                  // call kernel_main