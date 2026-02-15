// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __EFI_SIMPLE_TEXT_IN_H__
#define __EFI_SIMPLE_TEXT_IN_H__

#define SCAN_NULL      0x0000
#define SCAN_UP        0x0001
#define SCAN_DOWN      0x0002
#define SCAN_RIGHT     0x0003
#define SCAN_LEFT      0x0004
#define SCAN_HOME      0x0005
#define SCAN_END       0x0006
#define SCAN_INSERT    0x0007
#define SCAN_DELETE    0x0008
#define SCAN_PAGE_UP   0x0009
#define SCAN_PAGE_DOWN 0x000A
#define SCAN_F1        0x000B
#define SCAN_F2        0x000C
#define SCAN_F3        0x000D
#define SCAN_F4        0x000E
#define SCAN_F5        0x000F
#define SCAN_F6        0x0010
#define SCAN_F7        0x0011
#define SCAN_F8        0x0012
#define SCAN_F9        0x0013
#define SCAN_F10       0x0014
#define SCAN_ESC       0x0017

typedef struct efi_simple_text_input_protocol_s
    efi_simple_text_input_protocol_t;

typedef struct efi_input_key_s
{
    char16_t scan_code;
    uint16_t unicode_char;
} efi_input_key_t;

typedef efi_status_t(EFIAPI *efi_input_read_key_t)(
    efi_simple_text_input_protocol_t *this,
    efi_input_key_t *key
);

struct efi_simple_text_input_protocol_s
{
    efi_uint_t           buf;
    efi_input_read_key_t read_key_stroke;
    efi_event_t          wait_for_key;
};

#endif /* __EFI_SIMPLE_TEXT_IN_H__ */