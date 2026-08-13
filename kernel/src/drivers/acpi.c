// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <drivers/acpi.h>

struct acpi_description_header *
acpi_find_table(struct boot_info *boot_info, uint32_t signature)
{
    struct acpi_description_header *head;

    uint32_t i;
    for (i = 0; i < boot_info->sdt_entries; i++)
    {
        head = (void *)boot_info->sdt_baseaddr_array[i];
        if (head->signature == signature)
        {
            return head;
        }
    }
    return NULL;
}