// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright 2024 LinChenjun
 */

#ifndef __BOOTLOADER_CONFIG_H__
#define __BOOTLOADER_CONFIG_H__

UINT32 VideoModes[12][2] = {
    { 2560, 1600 }, { 1920, 1200 }, { 1920, 1080 }, { 1680, 1050 },
    { 1600, 1200 }, { 1440, 900 },  { 1280, 1024 }, { 1280, 800 },
    { 1280, 720 },  { 1024, 768 },  { 800, 600 },   { 0, 0 },
};


struct Files
{
    CHAR16           *Name;
    EFI_ALLOCATE_TYPE FileBufferType;
    file_info_t       Info;
};

struct Files Files[FILES_COUNT] = {
    { L"Kernel\\clfgkrnl.sys", AllocateAddress, { 0x100000, 0, 0x80000001 } },
    { L"Kernel\\typeface.ttf", AllocateAnyPages, { 0, 0, 0x80000002 } },
};

#endif