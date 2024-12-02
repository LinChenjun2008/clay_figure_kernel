/*
   Copyright 2024 LinChenjun

   本文件是Clay Figure Kernel的一部分。
   修改和/或再分发遵循GNU GPL version 3 (or any later version)
  
*/

#include <Efi.h>

EFI_STATUS ReadFile
(
    CHAR16 *FileName,
    EFI_PHYSICAL_ADDRESS *FileBufferBase,
    EFI_ALLOCATE_TYPE BufferType,
    UINT64 *FileSize
)
{
    EFI_FILE_PROTOCOL *FileHandle;
    EFI_STATUS Status = EFI_SUCCESS;
    UINTN HandleCount = 0;
    EFI_HANDLE *HandleBuffer;

    Status = gBS->LocateHandleBuffer
    (
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &HandleCount,
        &HandleBuffer
    );
    if (EFI_ERROR(Status))
    {
        return Status;
    }
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL *Root;
    UINTN Nr;
    for (Nr = 0;Nr < HandleCount;Nr++)
    {
        Status = gBS->OpenProtocol
        (
            HandleBuffer[Nr],
            &gEfiSimpleFileSystemProtocolGuid,
            (VOID**)&FileSystem,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );
        if (EFI_ERROR(Status))
        {
            return Status;
        }
        Status = FileSystem->OpenVolume
        (
            FileSystem,
            &Root
        );
        if (EFI_ERROR(Status))
        {
            return Status;
        }
        Status = Root->Open
        (
            Root,
            &FileHandle,
            FileName,
            EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );
        if (EFI_ERROR(Status))
        {
            if (Nr == HandleCount - 1)
                return Status;
        }
        else
        {
            break;
        }
    }

    EFI_FILE_INFO *FileInfo;
    UINTN InfoSize = sizeof(EFI_FILE_INFO) + sizeof(*FileName) * 256;
    Status = gBS->AllocatePool
    (
        EfiLoaderData,
        InfoSize,
        (VOID**)&FileInfo
    );
    if (EFI_ERROR(Status))
    {
        return Status;
    }
    Status = FileHandle->GetInfo
    (
        FileHandle,
        &gEfiFileInfoGuid,
        &InfoSize,
        FileInfo
    );
    if (EFI_ERROR(Status))
    {
        gBS->FreePool(FileInfo);
        return Status;
    }
    UINTN FilePageSize = (FileInfo->FileSize + 0x0fff) / 0x1000;
    EFI_PHYSICAL_ADDRESS *FileBufferAddress = FileBufferBase;
    Status = gBS->AllocatePages
    (
        BufferType,
        EfiLoaderData,
        FilePageSize,
        FileBufferAddress
    );
    if (BufferType == AllocateAnyPages)
    {
        if (EFI_ERROR(Status))
        {
            gBS->FreePool(FileInfo);
            return Status;
        }
    }
    UINTN ReadSize = FileInfo->FileSize;
    Status = FileHandle->Read
    (
        FileHandle,
        &ReadSize,
        (VOID*)*FileBufferAddress
    );
    *FileSize = FileInfo->FileSize;
    *FileBufferBase = *FileBufferAddress;
    gBS->FreePool(FileInfo);
    FileHandle->Close(FileHandle);
    Root->Close(Root);
    return Status;
}