// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#include <Efi.h>

#define ABS(x) (x > 0 ? x : -x)

EFI_STATUS SetVideoMode(UINT32 xsize, UINT32 ysize)
{
    UINTN                                 SizeOfInfo = 0;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN                                 i;
    UINT32                                Mode = 0;
    for (i = 0; i < Gop->Mode->MaxMode; i++)
    {
        Gop->QueryMode(Gop, i, &SizeOfInfo, &Info);
        if (xsize == Info->HorizontalResolution &&
            ysize == Info->VerticalResolution)
        {
            Mode = i;
            Gop->SetMode(Gop, Mode);
            return EFI_SUCCESS;
        }
    }
    return EFI_ERR;
}

static void CleanBlock(UINT32 x, UINT32 y, UINT32 x1, UINT32 y1)
{
    EFI_PHYSICAL_ADDRESS vram = Gop->Mode->FrameBufferBase;
    UINT32               i, j;
    for (i = x; i < x1; i++)
    {
        for (j = y; j < y1; j++)
        {
            *((UINT32 *)vram + j * Gop->Mode->Info->PixelsPerScanLine + i) = 0;
        }
    }
}

static void DisplayBlock(UINT32 x, UINT32 y, UINT32 x1, UINT32 y1, UINT32 color)
{
    EFI_PHYSICAL_ADDRESS vram = Gop->Mode->FrameBufferBase;
    UINT32               i, j;
    for (i = x; i < x1; i++)
    {
        for (j = y; j < y1; j++)
        {
            *((UINT32 *)vram + j * Gop->Mode->Info->PixelsPerScanLine + i) =
                color;
        }
    }
}

EFI_STATUS DisplayLogo()
{
    INT32 bx = (Gop->Mode->Info->HorizontalResolution - 400) / 2;
    INT32 by = (Gop->Mode->Info->VerticalResolution - 200) / 2;
    CleanBlock(bx - 5, by - 5, bx + 405, by + 205);
    DisplayBlock(bx, by, bx + 400, by + 200, 0x00ffffff);

    CleanBlock(bx + 98, by + 0, bx + 102, by + 200);
    CleanBlock(bx + 198, by + 0, bx + 202, by + 200);
    CleanBlock(bx + 298, by + 0, bx + 302, by + 200);

    // C
    CleanBlock(bx + 48, by + 48, bx + 52, by + 152);
    CleanBlock(bx + 48, by + 98, bx + 100, by + 102);

    // L
    CleanBlock(bx + 148, by + 0, bx + 200, by + 148);

    // F
    CleanBlock(bx + 248, by + 100, bx + 300, by + 200);
    CleanBlock(bx + 248, by + 48, bx + 300, by + 52);

    // G
    CleanBlock(bx + 348, by + 48, bx + 400, by + 52);
    CleanBlock(bx + 348, by + 48, bx + 352, by + 152);
    return EFI_SUCCESS;
}