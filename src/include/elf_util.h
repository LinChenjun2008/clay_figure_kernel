// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ELF_UTIL_H__
#define __ELF_UTIL_H__

int load_segment(
    uintptr_t  elf_file,
    uintptr_t *physical_base,
    uintptr_t *relocate_base,
    uintptr_t *entry
);

#endif /* __ELF_UTIL_H__ */