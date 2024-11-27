/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#ifndef __BOOTLOADER_CONFIG_H__
#define __BOOTLOADER_CONFIG_H__

#define HORIZONTAL_RESOLUTION 1920
#define VERTICAL_RESOLUTION   1080

struct Files
{
    CHAR16           *Name;
    EFI_ALLOCATE_TYPE FileBufferType;
    file_info_t       Info;
};

struct Files Files[] =
{
    {L"Kernel\\kernel.sys",AllocateAddress,{0x100000,0,0x80000001}},
};

#endif