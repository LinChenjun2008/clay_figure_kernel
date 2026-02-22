// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __DRIVER_ACPI_H__
#define __DRIVER_ACPI_H__

#pragma pack(1)
struct acpi_description_header
{
    uint32_t signature;
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    uint8_t  oem_id[6];
    uint64_t oem_table_id;
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
};
#pragma pack()

// xsdt.c
struct acpi_description_header *
xsdt_find_table(struct boot_info *boot_info, uint32_t signature);

#endif /* __DRIVER_ACPI_H__ */