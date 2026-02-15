// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */
#ifndef __EFI_LOADED_IMAGE_H__
#define __EFI_LOADED_IMAGE_H__

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    { 0x5B1B31A1,                      \
      0x9562,                          \
      0x11d2,                          \
      { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } }

extern efi_guid_t efi_loaded_image_protocol_guid;

#include <efi/protocol/device_path.h>
#include <efi/spec.h> // IMAGE_UNLOAD

typedef struct efi_loaded_image_protocol_s
{
    uint32_t                    revision;
    efi_handle_t                parent_handle;
    efi_system_table_t         *system_table;
    // source location of the image
    efi_handle_t                device_handle;
    efi_device_path_protocol_t *file_path;
    void                       *reserved;
    // image's load options
    uint32_t                    load_options_size;
    void                       *load_options;
    // location where image was loaded
    void                       *image_base;

    uint64_t          image_size;
    efi_memory_type_t image_code_type;

    efi_memory_type_t image_data_type;

    efi_image_unload_t unload;

} efi_loaded_image_protocol_t;

#endif /* __EFI_LOADED_IMAGE_H__ */