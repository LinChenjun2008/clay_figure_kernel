/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

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