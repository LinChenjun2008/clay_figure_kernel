#ifndef __BOOTLOADER_CONFIG_H__
#define __BOOTLOADER_CONFIG_H__

#define HORIZONTAL_RESOLUTION 1920
#define VERTICAL_RESOLUTION   1080

struct Files
{
    CHAR16 *Name;
    EFI_ALLOCATE_TYPE    FileBufferType;
    file_info_t Info;
};

struct Files Files[] =
{
    {L"Kernel\\kernel.sys",AllocateAddress,{0x100000,0,0x80000001}},
    {L"Kernel\\resource\\typeface.ttf",AllocateAnyPages,{0x800000,0,0x80000003}},
};

#endif