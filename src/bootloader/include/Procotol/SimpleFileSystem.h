#ifndef __EFI_SIMPLE_FILE_SYSTEM__
#define __EFI_SIMPLE_FILE_SYSTEM__

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
{ \
    0x964e5b22, 0x6459, 0x11d2, \
    {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
}

#define EFI_FILE_INFO_ID \
{ \
    0x9576e92, 0x6d3f, 0x11d2, \
    {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
}

#define EFI_FILE_MODE_READ      0x0000000000000001
#define EFI_FILE_MODE_WRITE     0x0000000000000002
#define EFI_FILE_MODE_CREATE    0x8000000000000000

typedef struct
{
  UINT64      Size;             /* 这是FileInfo的大小(包括FileName) */
  UINT64      FileSize;         /* 文件大小(单位:byet) */
  UINT64      PhysicalSize;     /* 文件在文件系统上所占用的物理空间大小 */

  EFI_TIME    CreateTime;       /* 文件创建时间 */
  EFI_TIME    LastAccessTime;   /* 文件最后一次访问时间 */
  EFI_TIME    ModificationTime; /* 文件最后一次修改时间 */

  UINT64      Attribute;        /* 文件属性 */

  CHAR16      FileName[1];      /* 文件名 */
} EFI_FILE_INFO;

extern EFI_GUID  gEfiSimpleFileSystemProtocolGuid;
extern EFI_GUID  gEfiFileInfoGuid;

typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;
typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_OPEN)
(
    EFI_FILE_PROTOCOL  *This,
    EFI_FILE_PROTOCOL **NewHandle, /* 被打开的文件handle */
    CHAR16             *FileName,  /* 文件名 */
    UINTN               OpenMode,  /* 打开文件的模式,支持:
                                    * READ(只读),
                                    * READ | WRITE(读写),
                                    * READ | WRITE | CREAT(读写,不存在则创建) 三种
                                    */
    UINTN               Attributes /* 文件属性(新建文件时) */
);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_CLOSE)
(
    EFI_FILE_PROTOCOL* This
);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_READ)
(
    EFI_FILE_PROTOCOL *This,
    UINTN             *BufferSize,
    VOID              *Buffer
);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_WRITE)
(
    EFI_FILE_PROTOCOL *This,
    UINTN             *BufferSize,
    VOID              *Buffer
);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_GET_INFO)
(
    EFI_FILE_PROTOCOL *This,
    EFI_GUID          *InformationType,
    UINTN             *BufferSize,
    VOID              *Buffer
);

struct _EFI_FILE_PROTOCOL
{
    UINT64 _buf;
    EFI_FILE_OPEN    Open;
    EFI_FILE_CLOSE   Close;
    UINTN _buf2; // Delet
    EFI_FILE_READ     Read;
    EFI_FILE_WRITE    Write;
    UINTN _buf3[2]; //GetPosition SetPosition
    EFI_FILE_GET_INFO GetInfo;
    UINTN _buf3_2[2];// SetInfo Flush
    UINTN _buf4[4]; //OpenEx ReadEx WriteEx FlushEx
};

typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME)
(
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
    EFI_FILE_PROTOCOL              **Root
);

struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
{
    UINTN                                       Revision;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume;
};

extern EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *gSFSP;

#endif