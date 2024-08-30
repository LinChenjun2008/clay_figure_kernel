/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.
Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

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