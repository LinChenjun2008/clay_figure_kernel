// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __EFI_GRAPHICS_OUTPUT_H__
#define __EFI_GRAPHICS_OUTPUT_H__

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    { 0x9042a9de,                         \
      0x23dc,                             \
      0x4a38,                             \
      { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } }

typedef struct efi_graphics_output_protocol_s efi_graphics_output_protocol_t;

typedef struct
{
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t reserved_mask;
} efi_pixel_bitmask_t;

typedef struct
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
} efi_graphci_output_blt_t;

typedef enum
{
    PIXEL_RED_GREEN_BLUE_RESERVED_8_BIT_PER_COLOR,
    PIXEL_BLUE_GREEN_RED_RESERVED_8_BIT_PER_COLOR,
    PIXEL_BIT_MASK,
    PIXEL_BLT_ONLY,
    PIXEL_FORMAT_MAX
} efi_graphics_pixel_format_t;

typedef struct
{
    uint32_t                    version;
    uint32_t                    horizontal_resolution;
    uint32_t                    vertical_resolution;
    efi_graphics_pixel_format_t pixel_format;
    efi_pixel_bitmask_t         pixel_information;
    uint32_t                    pixels_per_scan_line;
} efi_graphcis_output_mode_information_t;

typedef struct
{
    uint32_t                                max_mode;
    uint32_t                                mode;
    efi_graphcis_output_mode_information_t *info;
    efi_uint_t                              size_of_info;
    efi_physical_address_t                  frame_buffer_base;
    efi_uint_t                              frame_buffer_size;
} efi_graphics_output_protocol_mode_t;

typedef efi_status_t(EFIAPI *efi_graphics_output_protocol_query_mode_t)(
    efi_graphics_output_protocol_t *this,
    uint32_t                                 mode_number,
    efi_uint_t                              *size_of_info,
    efi_graphcis_output_mode_information_t **info
);

typedef efi_status_t(EFIAPI *efi_graphics_output_protocol_set_mode_t)(
    efi_graphics_output_protocol_t *this,
    uint32_t mode_number
);

struct efi_graphics_output_protocol_s
{
    efi_graphics_output_protocol_query_mode_t query_mode;
    efi_graphics_output_protocol_set_mode_t   set_mode;
    efi_uint_t                                buf[1];
    efi_graphics_output_protocol_mode_t      *mode;
};

extern efi_graphics_output_protocol_t *gop;

#endif /* __EFI_GRAPHICS_OUTPUT_H__ */