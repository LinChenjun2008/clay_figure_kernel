// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __DRIVER_ACPI_H__
#define __DRIVER_ACPI_H__

#include <efi.h>
#include <efi/acpi.h>

typedef efi_acpi_description_header_t acpi_description_header_t;

// xsdt.c
acpi_description_header_t *
xsdt_find_table(struct boot_info *boot_info, uint32_t signature);

#endif /* __DRIVER_ACPI_H__ */