// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __EFI_H__
#define __EFI_H__

// Attributes
#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020

#define EVT_TIMER                         0x80000000
#define EVT_RUNTIME                       0x40000000
#define EVT_NOTIFY_WAIT                   0x00000100
#define EVT_NOTIFY_SIGNAL                 0x00000200
#define EVT_SIGNAL_EXIT_BOOT_SERVICES     0x00000201
#define EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE 0x60000202

#include <efi/basetype.h>
#include <efi/spec.h>

#define RETURN_ERROR(STATUS_CODE) \
    ((efi_int_t)((efi_return_status_t)(STATUS_CODE)) < 0)

#define EFI_ERROR(A) RETURN_ERROR(A)

#define EFI_SUCCESS     0
#define EFI_ERR         0x8000000000000000
#define EFI_UNSUPPORTED (EFI_ERR | 3)

#include <efi/protocol/graphics_output.h>
#include <efi/protocol/simple_file_system.h>

extern efi_handle_t         image_handle;
extern efi_system_table_t  *system_table;
extern efi_boot_services_t *boot_services;

#endif /* __EFI_H__ */