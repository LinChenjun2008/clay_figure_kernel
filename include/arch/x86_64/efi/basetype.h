// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __EFI_BASE_TYPE_H__
#define __EFI_BASE_TYPE_H__

#include <efi/base.h>

typedef uint8_t boolen_t;

typedef efi_return_status_t efi_status_t;

typedef void *efi_handle_t;
typedef void *efi_event_t;

typedef efi_uint_t efi_physical_address_t;
typedef efi_uint_t efi_virtual_address_t;

#endif /* __EFI_BASE_TYPE_H__ */