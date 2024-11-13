#ifndef __EFI_ACPI_H__
#define __EFI_ACPI_H__


#pragma pack(1)
typedef struct
{
    UINT32  Signature;
    UINT32 Length;
    UINT8  Revision;
    UINT8  Checksum;
    UINT8  OemId[6];
    UINT64 OemTableId;
    UINT32 OemRevision;
    UINT32 CreatorId;
    UINT32 CreatorRevision;
} EFI_ACPI_DESCRIPTION_HEADER;

typedef struct
{
    EFI_ACPI_DESCRIPTION_HEADER Header;
    UINT32                      Entry;
} RSDT_TABLE;

typedef struct
{
    EFI_ACPI_DESCRIPTION_HEADER Header;
    UINT64                      Entry;
} XSDT_TABLE;

typedef struct
{
    UINT64  Signature;
    UINT8  Checksum;
    UINT8  OemId[6];
    UINT8  Revision;
    UINT32 RsdtAddress;
    UINT32 Length;
    UINT64 XsdtAddress;
    UINT8  ExtendedChecksum;
    UINT8  Reserved[3];
} EFI_ACPI_6_4_ROOT_SYSTEM_DESCRIPTION_POINTER;
#pragma pack()

#endif