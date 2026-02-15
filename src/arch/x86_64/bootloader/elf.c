// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include "bootloader.h"

#define ELF_ALLOCATE_PAGES(PAGES, ADDRESS)                    \
    boot_services->allocate_pages(                            \
        EFI_ALLOCATE_ADDRESS, EFI_LOADER_DATA, PAGES, ADDRESS \
    )

#define ELF_MEMSET(DEST, VALUE, SIZE) boot_services->set_mem(DEST, SIZE, VALUE)

#define ELF_MEMCPY(DEST, SRC, SIZE) boot_services->copy_mem(DEST, SRC, SIZE)

#define ELF_ERROR(STATUS) EFI_ERROR(STATUS)

#include "../../../elf_util.c"