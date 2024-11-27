/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#include <Efi.h>

#define ABS(x) (x > 0 ? x : -x)

EFI_STATUS SetVideoMode(UINT32 xsize,UINT32 ysize)
{
    UINTN SizeOfInfo = 0;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN i;
    UINT32 Mode = 0;
    UINTN sub = 0xffffffff;
    for(i = 0;i < Gop->Mode->MaxMode;i++)
    {
        Gop->QueryMode(Gop,i,&SizeOfInfo,&Info);
        if(ABS(xsize - Info->HorizontalResolution)
           + ABS(ysize -Info->VerticalResolution) < sub)
        {
            sub =  ABS(xsize - Info->HorizontalResolution)
                 + ABS(ysize - Info->VerticalResolution);
            Mode = i;
        }
    }
    if (Mode == Gop->Mode->MaxMode)
    {
        return EFI_ERR;
    }
    return Gop->SetMode(Gop,Mode);
}

static char logo[13][13] =\
{
    "#############",
    "# #          ",
    "# # #########",
    "# # #        ",
    "# # # #######",
    "# # # #      ",
    "# # # # #####",
    "# # # #     #",
    "# # # #######",
    "# # #        ",
    "# ###########",
    "#            ",
    "#############"
};

static void CleanBlock(UINT32 x,UINT32 y,UINT32 x1,UINT32 y1)
{
    EFI_PHYSICAL_ADDRESS vram = Gop->Mode->FrameBufferBase;
    UINT32 i,j;
    for (i = x;i < x1;i++)
    {
        for (j = y;j < y1;j++)
        {
            *((UINT32*)vram + j * Gop->Mode->Info->HorizontalResolution + i) = 0;
        }
    }
}

static void DisplayBlock(UINT32 x,UINT32 y,UINT32 x1,UINT32 y1)
{
    EFI_PHYSICAL_ADDRESS vram = Gop->Mode->FrameBufferBase;
    UINT32 i,j;
    for (i = x;i < x1;i++)
    {
        for (j = y;j < y1;j++)
        {
            *((UINT32*)vram + j * Gop->Mode->Info->HorizontalResolution + i) = 0xffffff;
        }
    }
}

EFI_STATUS DisplayLogo()
{
    UINT32 bx = (Gop->Mode->Info->HorizontalResolution - 130) / 2;
    INT32 by = (Gop->Mode->Info->VerticalResolution - 130) / 2;
    INT32 i,j;
    for (i = -1; i < 14;i++)
    {
        for (j = -1;j < 14;j++)
        {
            CleanBlock(bx + i * 10,by + j * 10,bx + (i + 1) * 10,by + (j + 1) * 10);
        }
    }
    for (i = 0; i < 13;i++)
    {
        for (j = 0;j < 13;j++)
        {
            if (logo[j][i] == '#')
            {
                DisplayBlock(bx + i * 10,by + j * 10,bx + (i + 1) * 10,by + (j + 1) * 10);
            }
        }
    }
    return EFI_SUCCESS;
}