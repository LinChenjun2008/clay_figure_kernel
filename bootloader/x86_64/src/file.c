// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <bootloader.h>
#include <ramfs.h>

#define ALIGN_PAD(X, ALIGN) (((ALIGN) - ((X) & ((ALIGN) - 1))) & ((ALIGN) - 1))

efi_status_t
read_file(char16_t *file_name, void **file_buffer_base, efi_uint_t *file_size)
{
    efi_status_t              status = EFI_SUCCESS;
    struct efi_file_protocol *file_handle;

    struct efi_file_protocol *root;

    struct efi_loaded_image_protocol *loaded_image;
    status = boot_services->handle_protocol(
        image_handle, &efi_loaded_image_protocol_guid, (void **)&loaded_image
    );
    if (EFI_ERROR(status))
    {
        printf(
            L"read_file: boot_services->handle_protocol(loaded_image): "
            L"ERROR(%d).\n\r",
            status
        );
        return status;
    }

    struct efi_simple_file_system_protocol *file_system;
    status = boot_services->handle_protocol(
        loaded_image->device_handle,
        &efi_simple_file_system_protocol_guid,
        (void **)&file_system
    );
    if (EFI_ERROR(status))
    {
        printf(
            L"read_file: boot_services->handle_protocol(file_system): "
            L"ERROR(%d).\n\r",
            status
        );
        return status;
    }

    status = file_system->open_volume(file_system, &root);
    if (EFI_ERROR(status))
    {
        printf(L"read_file: file_system->open_volume: ERROR(%d).\n\r", status);
        return status;
    }

    status = root->open(
        root,
        &file_handle,
        file_name,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );
    if (EFI_ERROR(status))
    {
        printf(L"read_file: root->open(%s): ERROR(%d).\n\r", file_name, status);
        return status;
    }

    struct efi_file_info *file_info;
    efi_uint_t file_info_size = sizeof(*file_info) + sizeof(*file_name) * 256;
    status                    = boot_services->allocate_pool(
        EFI_LOADER_DATA, file_info_size, (void **)&file_info
    );
    if (EFI_ERROR(status))
    {
        printf(
            L"read_file: boot_services->allocate_pool(file_info): "
            L"ERROR(%d).\n\r",
            status
        );
        return status;
    }

    status = file_handle->get_info(
        file_handle, &efi_file_info_guid, &file_info_size, file_info
    );
    if (EFI_ERROR(status))
    {
        printf(L"read_file: file_handle->get_info: ERROR(%d).\n\r", status);
        boot_services->free_pool(file_info);
        return status;
    }

    efi_uint_t file_page_size = (file_info->file_size + 0x0fff) >> 12;
    efi_physical_address_t file_buffer_address = 0;

    status = boot_services->allocate_pages(
        EFI_ALLOCATE_ANY_PAGES,
        EFI_LOADER_DATA,
        file_page_size,
        &file_buffer_address
    );
    if (EFI_ERROR(status))
    {
        printf(
            L"read_file: boot_services->allocate_pages(%lld): ERROR(%d).\n\r",
            file_page_size,
            status
        );
        boot_services->free_pool(file_info);
        return status;
    }

    boot_services->set_mem(
        (void *)file_buffer_address, file_page_size << 12, 0
    );

    efi_uint_t read_szie = file_info->file_size;
    status =
        file_handle->read(file_handle, &read_szie, (void *)file_buffer_address);
    if (EFI_ERROR(status))
    {
        printf(
            L"read_file: file_handle->read(%d): ERROR(%d).\n\r",
            read_szie,
            status
        );
        boot_services->free_pages(file_buffer_address, file_page_size);
    }
    else
    {
        *file_size        = file_info->file_size;
        *file_buffer_base = (void *)file_buffer_address;
    }

    boot_services->free_pool(file_info);
    file_handle->close(file_handle);
    root->close(root);
    return status;
}

void *ramfs_open(void *fs, const char *filename)
{
    struct file_header *header = fs;

    char    *name = NULL;
    uint8_t *file = NULL;

    while (1)
    {
        name = (char *)(header + 1);
        file = (uint8_t *)name + header->name_len;
        file += ALIGN_PAD(header->name_len, 4);
        if (strncmp(name, "TRAILER!!!", header->name_len) == 0)
        {
            return NULL;
        }
        if (strncmp(name, filename, header->name_len) == 0)
        {
            return file;
        }
        file += header->file_size;
        file += ALIGN_PAD(header->file_size, 4);
        header = (void *)file;
    }
    return NULL;
}