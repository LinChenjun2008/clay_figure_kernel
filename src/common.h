#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __BOOTLOADER__
typedef unsigned char          uint8_t;
typedef unsigned short         uint16_t;
typedef unsigned int           uint32_t;
typedef unsigned long long int uint64_t;

typedef unsigned long long int addr_t;
#endif

#pragma pack(1)
typedef struct
{
    addr_t    frame_buffer_base;
    uint32_t  horizontal_resolution;
    uint32_t  vertical_resolution;
} graph_info_t;

typedef struct
{
    uint64_t map_size;
    void    *buffer;
    uint64_t map_key;
    uint64_t descriptor_size;
    uint32_t descriptor_version;
} memory_map_t;

typedef struct
{
    addr_t    base_address;
    uint64_t  size;
    uint32_t  flag;
} file_info_t;

typedef struct
{
    uint64_t     magic;               // 5a 42 cb 16 13 d4 a6 2f
    memory_map_t memory_map;          // 内存描述符
    graph_info_t graph_info;          // 图形信息
    uint8_t      loaded_files;        // 已加载的文件数
    file_info_t *loaded_file;         // 已加载的文件
    void        *madt_addr;
} boot_info_t;

#pragma pack()

#endif