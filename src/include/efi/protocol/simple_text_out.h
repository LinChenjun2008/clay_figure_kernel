// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __EFI_SIMPLE_TEXT_OUT_H__
#define __EFI_SIMPLE_TEXT_OUT_H__

typedef struct efi_simple_text_output_protocol_s
    efi_simple_text_output_protocol_t;

typedef efi_status_t(EFIAPI *efi_text_string_t)(
    efi_simple_text_output_protocol_t *this,
    char16_t *string
);

typedef efi_status_t(EFIAPI *efi_text_test_string_t)(
    efi_simple_text_output_protocol_t *this,
    char16_t *string
);

typedef efi_status_t(EFIAPI *efi_text_query_mode_t)(
    efi_simple_text_output_protocol_t *this,
    efi_uint_t  mode_number,
    efi_uint_t *columns,
    efi_uint_t *rows
);

typedef efi_status_t(EFIAPI *efi_text_set_mode_t)(
    efi_simple_text_output_protocol_t *this,
    efi_uint_t mode_number
);

typedef efi_status_t(EFIAPI *efi_text_set_attribute_t)(
    efi_simple_text_output_protocol_t *this,
    efi_uint_t attribute
);

typedef efi_status_t(EFIAPI *efi_text_clear_screen_t)(
    efi_simple_text_output_protocol_t *this
);

typedef struct simple_text_output_mode
{
    efi_int_t    max_mode;
    efi_int_t    mode;
    efi_int_t    attribute;
    efi_int_t    cursor_column;
    efi_int_t    cursor_row;
    efi_boolen_t cursor_visible;
} simple_text_output_mode_t;

struct efi_simple_text_output_protocol_s
{
    efi_uint_t                 _buf;
    efi_text_string_t          output_string;
    efi_text_test_string_t     test_string;
    efi_text_query_mode_t      query_mode;
    efi_text_set_mode_t        set_mode;
    efi_text_set_attribute_t   set_attribute;
    efi_text_clear_screen_t    clear_screen;
    efi_uint_t                 _buf4[2];
    simple_text_output_mode_t *mode;
};

extern efi_guid_t efi_simple_text_out_protocol_guid;

#endif /* __EFI_SIMPLE_TEXT_OUT_H__ */