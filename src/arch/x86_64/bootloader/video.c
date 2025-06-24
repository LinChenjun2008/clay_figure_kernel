/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

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

// #define OLD_LOGO
#ifdef OLD_LOGO
static char logo[13][13] = { "#############", "# #          ", "# # #########",
                             "# # #        ", "# # # #######", "# # # #      ",
                             "# # # # #####", "# # # #     #", "# # # #######",
                             "# # #        ", "# ###########", "#            ",
                             "#############" };
#endif

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
#ifdef OLD_LOGO
    UINT32 bx = (Gop->Mode->Info->HorizontalResolution - 130) / 2;
    INT32  by = (Gop->Mode->Info->VerticalResolution - 130) / 2;
    INT32  i, j;
    for (i = -1; i < 14; i++)
    {
        for (j = -1; j < 14; j++)
        {
            CleanBlock(
                bx + i * 10, by + j * 10, bx + (i + 1) * 10, by + (j + 1) * 10
            );
        }
    }
    for (i = 0; i < 13; i++)
    {
        for (j = 0; j < 13; j++)
        {
            if (logo[j][i] == '#')
            {
                DisplayBlock(
                    bx + i * 10,
                    by + j * 10,
                    bx + (i + 1) * 10,
                    by + (j + 1) * 10,
                    0x00ffffff
                );
            }
        }
    }
#else
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

#endif
    return EFI_SUCCESS;
}