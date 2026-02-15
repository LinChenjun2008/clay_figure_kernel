// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __EFI_BASE_H__
#define __EFI_BASE_H__

#include <base.h>

#ifdef EFIAPI
#elif defined(_MSC_EXTENSIONS)
#    define EFIAPI __cdecl
#elif defined(__GNUC__)
#    define EFIAPI __attribute__((ms_abi))
#else
#    define EFIAPI
#endif

typedef unsigned char efi_boolen_t;
typedef uint64_t      efi_uint_t;
typedef int64_t       efi_int_t;

typedef struct
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} efi_guid_t;

typedef efi_uint_t efi_return_status_t;

#endif /* __EFI_BASE_H__ */
