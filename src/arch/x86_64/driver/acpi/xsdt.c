// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/driver/acpi.h>

acpi_description_header_t *
xsdt_find_table(boot_info_t *boot_info, uint32_t signature)
{
    acpi_description_header_t *head;

    uint32_t i;
    for (i = 0; i < boot_info->sdt_entries; i++)
    {
        head = (acpi_description_header_t *)boot_info->sdt_baseaddr_array[i];
        if (head->signature == signature)
        {
            return head;
        }
    }
    return NULL;
}