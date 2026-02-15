// SPDX-license-identifier: GPL-3.0-or-later
/**
 * copyright (C) 2026 linchenjun
 */

#ifndef __EFI_BASE_TYPE_H__
#define __EFI_BASE_TYPE_H__

#include <efi/base.h>

typedef efi_return_status_t efi_status_t;

typedef void *efi_handle_t;
typedef void *efi_event_t;

typedef efi_uint_t efi_physical_address_t;
typedef efi_uint_t efi_virtual_address_t;

typedef struct
{
    uint16_t year;   /* 1900 - 9999       */
    uint8_t  month;  /*    1 - 12         */
    uint8_t  day;    /*    1 - 31         */
    uint8_t  hour;   /*    0 - 23         */
    uint8_t  minute; /*    0 - 59         */
    uint8_t  second; /*    0 - 59         */
    uint8_t  pad1;
    uint32_t nanosecond; /*    0 - 999,999,999*/
    int16_t  time_zone;
    uint8_t  daylight;
    uint8_t  pad2;
} efi_time_t;

#endif /* __EFI_BASE_TYPE_H__ */