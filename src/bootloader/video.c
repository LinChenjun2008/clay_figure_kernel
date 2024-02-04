#include <Efi.h>

#define ABS(x) (x > 0 ? x : -x)

EFI_STATUS SetVideoMode(UINT32 xsize,UINT32 ysize)
{
    UINTN SizeOfInfo = 0;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN i;
    int Mode = 0;
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
    return Gop->SetMode(Gop,Mode);
}
