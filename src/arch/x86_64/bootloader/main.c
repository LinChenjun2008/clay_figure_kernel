// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include "bootloader.h"

struct efi_system_table        *system_table;
struct efi_boot_services       *boot_services;
efi_graphics_output_protocol_t *gop;
efi_handle_t                    image_handle;

struct efi_guid efi_graphics_output_protocol_guid =
    EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
struct efi_guid efi_loaded_image_protocol_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
struct efi_guid efi_simple_file_system_protocol_guid =
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
struct efi_guid efi_file_info_guid  = EFI_FILE_INFO_ID;
struct efi_guid efi_acpi_table_guid = EFI_ACPI_TABLE_GUID;

efi_status_t EFIAPI
efi_main(efi_handle_t in_image_handle, struct efi_system_table *in_system_table)
{
    efi_status_t status = EFI_SUCCESS;

    image_handle = in_image_handle;
    system_table = in_system_table;

    boot_services = system_table->boot_services;
    boot_services->locate_protocol(
        &efi_graphics_output_protocol_guid, NULL, (void **)&gop
    );

    // prepare boot info
    struct boot_info *boot_info = NULL;

    status = boot_services->allocate_pages(
        EFI_ALLOCATE_ANY_PAGES,
        EFI_LOADER_DATA,
        (sizeof(*boot_info) + 0xfff) >> 12,
        (efi_physical_address_t *)&boot_info
    );
    if (EFI_ERROR(status))
    {
        printf(
            L"boot_services->allocate_pages: cannot alloc memory for "
            L"boot_info.\n\r"
        );
        return status;
    }
    boot_services->set_mem(boot_info, sizeof(boot_info), 0);

    efi_uint_t             img_size;
    efi_physical_address_t img_base;
    status = read_file(L"kernel\\initramfs.img", &img_base, &img_size);
    if (EFI_ERROR(status))
    {
        printf(L"Open kernel/initramfs failed: ERROR(%d).\n\r", status);
        return status;
    }
    boot_info->initramfs      = (void *)img_base;
    boot_info->initramfs_size = img_size;

    if (ramfs_check((void *)img_base) < 0)
    {
        printf(L"check ramfs failed!\n\r");
        return EFI_ERR;
    }
    struct ramfs_file fp;
    if (ramfs_open((void *)img_base, "config", &fp) < 0)
    {
        printf(L"ramfs_open(config): failed.\n\r");
        return EFI_ERR;
    }
    parse_config(&fp);

    set_video_mode();

    struct graphic_info *graphic_info = &boot_info->graphic_info;
    efi_graphcis_output_mode_information_t *mode_info = gop->mode->info;

    graphic_info->frame_buffer_base     = gop->mode->frame_buffer_base;
    graphic_info->horizontal_resolution = mode_info->horizontal_resolution;
    graphic_info->vertical_resolution   = mode_info->vertical_resolution;
    graphic_info->pixel_per_scanline    = mode_info->pixels_per_scan_line;

    status = read_acpi_tables(boot_info);
    if (EFI_ERROR(status))
    {
        printf(L"read_acpi_tables: ERROR(%d).\n\r", status);
        return status;
    }

    if (ramfs_open((void *)img_base, "clfgkrnl", &fp) < 0)
    {
        printf(L"ramfs_open(kernel): failed.\n\r");
        return EFI_ERR;
    }

    SYSV_ABI int (*kernel_entry)(struct boot_info *, void *);
    uintptr_t physical_base = 0x100000;
    uintptr_t relocate_base = KERNEL_TEXT_BASE;
    load_segment(
        (uintptr_t)fp.data,
        &physical_base,
        &relocate_base,
        (uintptr_t *)&kernel_entry
    );
    boot_info->relocate_base = relocate_base;

    // Create page table
    uintptr_t *page_table_pos;
    status = create_page_table(&page_table_pos);
    if (EFI_ERROR(status))
    {
        printf(L"create_page_table: ERROR(%d).\n\r", status);
    }
    boot_info->page_table_pos = page_table_pos;

    // Allocate kernel stack (4kib)
    efi_physical_address_t kstack;
    status = boot_services->allocate_pages(
        EFI_ALLOCATE_ANY_PAGES, EFI_LOADER_DATA, 1, &kstack
    );
    if (EFI_ERROR(status))
    {
        printf(
            L"boot_services->allocate_pages(kstack): ERROR(%d).\n\r", status
        );
        return status;
    }
    boot_info->stack_base  = kstack;
    boot_info->stack_pages = 1;

    // Memory map.
    boot_info->memory_map.map_size           = 4096 * 4;
    boot_info->memory_map.buffer             = NULL;
    boot_info->memory_map.map_key            = 0;
    boot_info->memory_map.descriptor_size    = 0;
    boot_info->memory_map.descriptor_version = 0;

    status = get_memory_map(&boot_info->memory_map);
    if (EFI_ERROR(status))
    {
        printf(L"get_memory_map: ERROR(%d).\n\r");
        return status;
    }

    status = boot_services->exit_boot_services(
        image_handle, boot_info->memory_map.map_key
    );
    if (EFI_ERROR(status))
    {
        return status;
    }

    kernel_entry(boot_info, PHYS_TO_VIRT(kstack + PG_SIZE));
    while (1);
    return status;
}