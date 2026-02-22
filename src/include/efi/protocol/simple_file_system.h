// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __EFI_SIMPLE_FILE_SYSTEM_H__
#define __EFI_SIMPLE_FILE_SYSTEM_H__

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    { 0x964e5b22,                            \
      0x6459,                                \
      0x11d2,                                \
      { 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

#define EFI_FILE_INFO_ID \
    { 0x9576e92,         \
      0x6d3f,            \
      0x11d2,            \
      { 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

#define EFI_FILE_MODE_READ   0x0000000000000001
#define EFI_FILE_MODE_WRITE  0x0000000000000002
#define EFI_FILE_MODE_CREATE 0x8000000000000000

typedef struct efi_file_info
{
    uint64_t size;          /* 这是file_info的大小(包括file_name) */
    uint64_t file_size;     /* 文件大小(单位:byet) */
    uint64_t physical_size; /* 文件在文件系统上所占用的物理空间大小 */

    efi_time_t create_time;       /* 文件创建时间 */
    efi_time_t last_access_time;  /* 文件最后一次访问时间 */
    efi_time_t modification_time; /* 文件最后一次修改时间 */

    uint64_t attribute; /* 文件属性 */

    char16_t file_name[1]; /* 文件名 */
} efi_file_info_t;

extern struct efi_guid efi_simple_file_system_protocol_guid;
extern struct efi_guid efi_file_info_guid;

typedef struct efi_file_protocol_s efi_file_protocol_t;
typedef struct efi_simple_file_system_protocol_s
    efi_simple_file_system_protocol_t;

typedef efi_status_t(EFIAPI *efi_file_open_t)(
    efi_file_protocol_t *this,
    efi_file_protocol_t **new_handle, /* 被打开的文件handle */
    char16_t             *file_name,  /* 文件名 */
    efi_uint_t            open_mode,  /* 打开文件的模式,支持:
                                    * READ(只读),
                                    * READ | WRITE(读写),
                                    * READ | WRITE | CREAT(读写,不存在则创建) 三种
                                    */
    efi_uint_t            attributes  /* 文件属性(新建文件时) */
);

typedef efi_status_t(EFIAPI *efi_file_close_t)(efi_file_protocol_t *this);

typedef efi_status_t(EFIAPI *efi_file_read_t)(
    efi_file_protocol_t *this,
    efi_uint_t *buffer_size,
    void       *buffer
);

typedef efi_status_t(EFIAPI *efi_file_write_t)(
    efi_file_protocol_t *this,
    efi_uint_t *buffer_size,
    void       *buffer
);

typedef efi_status_t(EFIAPI *efi_file_get_info_t)(
    efi_file_protocol_t *this,
    struct efi_guid *information_type,
    efi_uint_t      *buffer_size,
    void            *buffer
);

struct efi_file_protocol_s
{
    uint64_t            buf;
    efi_file_open_t     open;
    efi_file_close_t    close;
    efi_uint_t          buf2; // delet
    efi_file_read_t     read;
    efi_file_write_t    write;
    efi_uint_t          buf3[2]; //get_position set_position
    efi_file_get_info_t get_info;
    efi_uint_t          buf3_2[2]; // set_info flush
    efi_uint_t          buf4[4];   //open_ex read_ex write_ex flush_ex
};

typedef efi_status_t(EFIAPI *efi_simple_file_system_protocol_open_volume_t)(
    efi_simple_file_system_protocol_t *this,
    efi_file_protocol_t **root
);

struct efi_simple_file_system_protocol_s
{
    efi_uint_t                                    revision;
    efi_simple_file_system_protocol_open_volume_t open_volume;
};

extern efi_simple_file_system_protocol_t *sfsp;

#endif /* __EFI_SIMPLE_FILE_SYSTEM_H__ */