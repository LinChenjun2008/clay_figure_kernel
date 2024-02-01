#ifndef __EFI_GRAPHICS_OUTPUT__
#define __EFI_GRAPHICS_OUTPUT__

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
{ \
    0x9042a9de, 0x23dc, 0x4a38, \
    {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } \
}

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct
{
  UINT32    RedMask;
  UINT32    GreenMask;
  UINT32    BlueMask;
  UINT32    ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct
{
  UINT8    Blue;
  UINT8    Green;
  UINT8    Red;
  UINT8    Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef enum
{
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
}  EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct
{
    UINT32                    Version;
    UINT32                    HorizontalResolution;
    UINT32                    VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK         PixelInformation;
    UINT32                    PixelsPerScanLine;
}  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct
{
    UINT32                                MaxMode;
    UINT32                                Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN                                 SizeOfInfo;
    EFI_PHYSICAL_ADDRESS                  FrameBufferBase;
    UINTN                                 FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)
(
    EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
    UINT32                                 ModeNumber,
    UINTN                                 *SizeOfInfo,
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
);

typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)
(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    UINT32                        ModeNumber
);

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE QueryMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE   SetMode;
    UINTN _buf[1];
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE  *Mode;
};

extern EFI_GRAPHICS_OUTPUT_PROTOCOL       *Gop;

#endif