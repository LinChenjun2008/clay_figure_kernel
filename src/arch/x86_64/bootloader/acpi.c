// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include "bootloader.h"

efi_status_t read_acpi_tables(struct boot_info *boot_info)
{
    efi_status_t status = EFI_SUCCESS;

    // Read acpi table
    // find rsdp
    efi_configuration_table_t *config_table = system_table->configuration_table;
    struct efi_acpi_6_4_root_system_description_pointer *rsdp;

    uint32_t i;
    for (i = 0; i < system_table->number_of_table_entries; i++)
    {
        if (compare_guid(&config_table->vendor_guid, &efi_acpi_table_guid))
        {
            rsdp = config_table->vendor_table;
            if (rsdp->revision == 2)
            {
                break;
            }
        }
        config_table++;
    }

    // read sdt
    struct xsdt_table *xsdt = (void *)rsdp->xsdt_address;
    uint32_t  sdt_entries   = (xsdt->header.length - sizeof(xsdt->header)) / 8;
    uint64_t *point_to_othre_sdt = &xsdt->entry;


    status = boot_services->allocate_pool(
        EFI_LOADER_DATA,
        sizeof(*boot_info->sdt_baseaddr_array) * sdt_entries,
        (void **)&boot_info->sdt_baseaddr_array
    );
    if (EFI_ERROR(status))
    {
        printf(
            L"boot_services->allocate_pool: cannot alloc memory for "
            L"sdt_baseaddr_array.\n\r"
        );
        return status;
    }
    boot_info->sdt_entries = sdt_entries;

    for (i = 0; i < sdt_entries; i++)
    {
        struct efi_acpi_description_header *h;
        h = (struct efi_acpi_description_header *)point_to_othre_sdt[i];

        status = boot_services->allocate_pool(
            EFI_LOADER_DATA,
            h->length,
            (void **)&boot_info->sdt_baseaddr_array[i]
        );
        if (EFI_ERROR(status))
        {
            printf(
                L"boot_services->allocate_pool: cannot alloc memory for "
                L"sdt_baseaddr_array[%d].\n\r",
                i
            );
            return status;
        }
        boot_services->copy_mem(boot_info->sdt_baseaddr_array[i], h, h->length);
    }
    return status;
}