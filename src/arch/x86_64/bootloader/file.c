// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#include <Efi.h>

EFI_STATUS ReadFile(
    CHAR16               *FileName,
    EFI_PHYSICAL_ADDRESS *FileBufferBase,
    EFI_ALLOCATE_TYPE     BufferType,
    UINT64               *FileSize
)
{
    EFI_FILE_PROTOCOL *FileHandle;
    EFI_STATUS         Status      = EFI_SUCCESS;
    UINTN              HandleCount = 0;
    EFI_HANDLE        *HandleBuffer;

    Status = gBS->LocateHandleBuffer(
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
    EFI_FILE_PROTOCOL               *Root;
    UINTN                            i;
    i = 0;
    do
    {
        Status = gBS->OpenProtocol(
            HandleBuffer[i++],
            &gEfiSimpleFileSystemProtocolGuid,
            (VOID **)&FileSystem,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );

        if (EFI_ERROR(Status))
        {
            return Status;
        }
        Status = FileSystem->OpenVolume(FileSystem, &Root);
        if (EFI_ERROR(Status))
        {
            return Status;
        }
        Status = Root->Open(
            Root,
            &FileHandle,
            FileName,
            EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );
    } while (EFI_ERROR(Status) && i < HandleCount);
    if (EFI_ERROR(Status))
    {
        return Status;
    }
    EFI_FILE_INFO *FileInfo;
    UINTN          InfoSize = sizeof(EFI_FILE_INFO) + sizeof(*FileName) * 256;
    Status = gBS->AllocatePool(EfiLoaderData, InfoSize, (VOID **)&FileInfo);
    if (EFI_ERROR(Status))
    {
        return Status;
    }
    Status =
        FileHandle->GetInfo(FileHandle, &gEfiFileInfoGuid, &InfoSize, FileInfo);
    if (EFI_ERROR(Status))
    {
        gBS->FreePool(FileInfo);
        return Status;
    }
    UINTN                 FilePageSize = (FileInfo->FileSize + 0x0fff) / 0x1000;
    EFI_PHYSICAL_ADDRESS *FileBufferAddress = FileBufferBase;
    Status                                  = gBS->AllocatePages(
        BufferType, EfiLoaderData, FilePageSize, FileBufferAddress
    );
    gBS->SetMem((VOID *)*FileBufferAddress, FilePageSize * 0x1000, 0);
    if (BufferType == AllocateAnyPages)
    {
        if (EFI_ERROR(Status))
        {
            gBS->FreePool(FileInfo);
            return Status;
        }
    }
    UINTN ReadSize = FileInfo->FileSize;
    Status =
        FileHandle->Read(FileHandle, &ReadSize, (VOID *)*FileBufferAddress);
    *FileSize       = FileInfo->FileSize;
    *FileBufferBase = *FileBufferAddress;
    gBS->FreePool(FileInfo);
    FileHandle->Close(FileHandle);
    Root->Close(Root);
    return Status;
}