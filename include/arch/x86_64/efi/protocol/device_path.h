// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __EFI_DEVICE_PATH_H__
#define __EFI_DEVICE_PATH_H__

#define EFI_DEVICE_PATH_PROTOCOL_GUID \
    { 0x09576e91,                     \
      0x6d3f,                         \
      0x11d2,                         \
      { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

struct efi_device_path_protocol
{
    uint8_t type;
    uint8_t sub_type;
    uint8_t length[2];
};

#endif /* __EFI_DEVICE_PATH_H__ */