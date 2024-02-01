#ifndef __EFI_BASE_TYPE__
#define __EFI_BASE_TYPE__

#include <Base.h>

typedef RETURN_STATUS EFI_STATUS;

typedef GUID EFI_GUID;

typedef VOID* EFI_HANDLE;

typedef VOID* EFI_EVENT;

typedef UINT64 EFI_PHYSICAL_ADDRESS;

typedef UINT64 EFI_VIRTUAL_ADDRESS;

typedef struct
{
    UINT16 Year;       /* 1900 - 9999       */
    UINT8  Month;      /*    1 - 12         */
    UINT8  Day;        /*    1 - 31         */
    UINT8  Hour;       /*    0 - 23         */
    UINT8  Minute;     /*    0 - 59         */
    UINT8  Second;     /*    0 - 59         */
    UINT8  Pad1;
    UINT32 Nanosecond; /*    0 - 999,999,999*/
    INT16  TimeZone;
    UINT8  Daylight;
    UINT8  Pad2;
} EFI_TIME;

#endif