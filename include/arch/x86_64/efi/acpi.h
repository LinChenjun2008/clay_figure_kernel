// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __EFI_ACPI_H__
#define __EFI_ACPI_H__

#define ACPI_TABLE_GUID \
    { 0xeb9d2d30,       \
      0x2d88,           \
      0x11d3,           \
      { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

#define EFI_ACPI_TABLE_GUID \
    { 0x8868e871,           \
      0xe4f1,               \
      0x11d3,               \
      { 0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }

#define ACPI_10_TABLE_GUID     ACPI_TABLE_GUID
#define EFI_ACPI_20_TABLE_GUID EFI_ACPI_TABLE_GUID

extern struct efi_guid efi_acpi_table_guid;
extern struct efi_guid efi_acpi_10_table_guid;
extern struct efi_guid efi_acpi_20_table_guid;

#pragma pack(1)

struct efi_acpi_description_header
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

struct rsdt_table
{
    struct efi_acpi_description_header header;
    uint32_t                           entry;
};

struct xsdt_table
{
    struct efi_acpi_description_header header;
    uint64_t                           entry;
};

struct efi_acpi_6_4_root_system_description_pointer
{
    uint64_t signature;
    uint8_t  checksum;
    uint8_t  oem_id[6];
    uint8_t  revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t  extended_checksum;
    uint8_t  reserved[3];
};

struct efi_acpi_data_table
{
    struct efi_acpi_description_header header;
    struct efi_guid                    identifier;
    uint16_t                           data_offset;
};

#pragma pack()

#endif /* __EFI_ACPI_H__ */