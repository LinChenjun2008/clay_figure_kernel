#ifndef __EFI_SPEC__
#define __EFI_SPEC__

#include <Uefi/UefiMultiPhase.h>

#include <Procotol/SimpleTextIn.h>
#include <Procotol/SimpleTextOut.h>

typedef enum
{
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType,
} EFI_ALLOCATE_TYPE;

typedef struct
{
    UINT32                  Type;
    EFI_PHYSICAL_ADDRESS    PhysicalStart;
    EFI_VIRTUAL_ADDRESS     VirtualStart;
    UINT64                  NumberOfPages;
    UINT64                  Attribute;
} __attribute__((aligned(16))) EFI_MEMORY_DESCRIPTOR;

typedef
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_PAGES)
(
    EFI_ALLOCATE_TYPE     Type,
    EFI_MEMORY_TYPE       MemoryType,
    UINTN                 Pages,
    EFI_PHYSICAL_ADDRESS* Memory
);

typedef
EFI_STATUS
(EFIAPI *EFI_FREE_PAGES)
(
    EFI_PHYSICAL_ADDRESS Memory,
    UINTN                Pages
);

typedef
EFI_STATUS
(EFIAPI *EFI_GET_MEMORY_MAP)
(
    UINTN                 *MemoryMapSize,
    EFI_MEMORY_DESCRIPTOR *MemoryMap,
    UINTN                 *MapKey,
    UINTN                 *DescriptorSize,
    UINT32                *DescriptorVersion
);

typedef
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_POOL)
(
  IN  EFI_MEMORY_TYPE   PoolType,
  IN  UINTN             Size,
  OUT VOID            **Buffer
);

typedef
EFI_STATUS
(EFIAPI *EFI_FREE_POOL)
(
  IN  VOID *Buffer
);

typedef
EFI_STATUS
(EFIAPI *EFI_WAIT_FOR_EVENT)
(
    UINTN  NumberOfEvents,
    VOID **Event,
    UINTN *Index
);

typedef
EFI_STATUS
(EFIAPI *EFI_EXIT_BOOT_SERVICES)
(
    EFI_HANDLE ImageHandle,
    UINTN      MapKey
);

typedef
EFI_STATUS
(EFIAPI *EFI_SET_WATCHDOG_TIMER)
(
    UINTN   Timeout,
    UINTN   WatchdogCode,
    UINTN   DataSize,
    UINT16 *WatchdogData
);

typedef
EFI_STATUS
(EFIAPI *EFI_OPEN_PROTOCOL)
(
    EFI_HANDLE   Handle,
    EFI_GUID    *Protocol,
    VOID      **Interface,
    EFI_HANDLE   AgentHandle,
    EFI_HANDLE   ControllerHandle,
    UINT32      Attributes
);

typedef
EFI_STATUS
(EFIAPI *EFI_CLOSE_PROTOCOL)
(
    EFI_HANDLE Handle,
    EFI_GUID  *Protocol,
    EFI_HANDLE AgentHandle,
    EFI_HANDLE ControllerHandle
);

typedef enum
{
  AllHandles,
  ByRegisterNotify,
  ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_HANDLE_BUFFER)
(
    EFI_LOCATE_SEARCH_TYPE SearchType,
    EFI_GUID              *Protocol,
    VOID                  *SearchKey,
    UINTN                 *NoHandles,
   EFI_HANDLE            **Buffer
);

typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_PROTOCOL)
(
    EFI_GUID *Protocol,
    VOID     *Registration,
    VOID    **Interface
);


typedef
VOID
(EFIAPI *EFI_COPY_MEM)
(
    VOID     *Destination,
    VOID     *Source,
    UINTN    Length
);

typedef
VOID
(EFIAPI *EFI_SET_MEM)
(
    VOID     *Buffer,
    UINTN    Size,
    UINT8    Value
);


/// Uefi Runtime services
typedef struct
{
    UINT8 _buf_rs1[24];

    //
    // Time Services
    //
    UINTN _buf_rs2[4];

    //
    // Virtual Memory Services
    //
    UINTN _buf_rs3[2];

    //
    // Variable Services
    //
    UINTN _buf_rs4[3];

    //
    // Miscellaneous Services
    //
    UINTN _buf_rs5;
    void (*ResetSystem)
    (
        EFI_RESET_TYPE ResetType,
        UINTN ResetStatus,
        UINTN DataSize,
        VOID* ResetData
    );
} EFI_RUNTIME_SERVICES;



typedef struct
    {
    char _buf1[24];

    // Task Priority Services
    UINTN _buf2[2];

    // Memory Services
    EFI_ALLOCATE_PAGES       AllocatePages;
    EFI_FREE_PAGES           FreePages;
    EFI_GET_MEMORY_MAP       GetMemoryMap;
    EFI_ALLOCATE_POOL        AllocatePool;
    EFI_FREE_POOL            FreePool;

    // Event & Timer Services
    UINTN _buf4[2];
    EFI_WAIT_FOR_EVENT       WaitForEvent;
    UINTN _buf4_2[3];

    // Protocol Handler Services
    UINTN _buf5[9];

    // Image Services
    UINTN _buf6[4];
    EFI_EXIT_BOOT_SERVICES   ExitBootServices;
    // Miscellaneous Services
    UINTN _buf7[2];
    EFI_SET_WATCHDOG_TIMER   SetWatchdogTimer;

    // DriverSupport Services
    UINTN _buf8[2];

    // Open and Close Protocol Services

    EFI_OPEN_PROTOCOL        OpenProtocol;
    EFI_CLOSE_PROTOCOL       CloseProtocol;
    UINTN _buf9[1];

    // Library Services
    UINTN _buf10[1];
    EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL      LocateProtocol;
    UINTN _buf_10_2[2];
    // 32-bit CRC Services
    UINTN _buf11;

    // Miscellaneous Services
    EFI_COPY_MEM             CopyMem;
    EFI_SET_MEM              SetMem;
    UINTN _buf12;
} EFI_BOOT_SERVICES;

typedef struct
{
    EFI_GUID  VendorGuid;
    VOID     *VendorTable;
} EFI_CONFIGURATION_TABLE;

typedef struct
{
    UINT8 _buf1[44];
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *ConIn;
    UINTN _buf2;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    unsigned long long _buf3[2];
    EFI_RUNTIME_SERVICES            *RuntimeServices;
    EFI_BOOT_SERVICES               *BootServices;
    UINTN                            NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE         *ConfigurationTable;
} EFI_SYSTEM_TABLE;

#endif