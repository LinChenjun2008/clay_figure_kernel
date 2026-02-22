// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __EFI_SPEC_H__
#define __EFI_SPEC_H__

#include <efi/multiphase.h>
#include <efi/protocol/simple_text_in.h>
#include <efi/protocol/simple_text_out.h>

enum efi_allocate_type
{
    EFI_ALLOCATE_ANY_PAGES,
    EFI_ALLOCATE_MAX_ADDRESS,
    EFI_ALLOCATE_ADDRESS,
    EFI_MAX_ALLOCATE_TYPE
};

struct efi_memory_descriptor
{
    uint32_t               type;
    efi_physical_address_t physical_start;
    efi_virtual_address_t  virtual_start;
    uint64_t               number_of_pages;
    uint64_t               attribute;
} ALIGNED(16);

typedef efi_status_t(EFIAPI *efi_allocate_pages_t)(
    enum efi_allocate_type  type,
    enum efi_memory_type    memory_type,
    efi_uint_t              pages,
    efi_physical_address_t *memory
);

typedef efi_status_t(EFIAPI *efi_free_pages_t)(
    efi_physical_address_t memory,
    efi_uint_t             pages
);

typedef efi_status_t(EFIAPI *efi_get_memory_map_t)(
    efi_uint_t                   *memory_map_size,
    struct efi_memory_descriptor *memory_map,
    efi_uint_t                   *map_key,
    efi_uint_t                   *descriptor_size,
    uint32_t                     *descriptor_version
);

typedef efi_status_t(EFIAPI *efi_allocate_pool_t)(
    enum efi_memory_type pool_type,
    efi_uint_t           size,
    void               **buffer
);

typedef efi_status_t(EFIAPI *efi_free_pool_t)(void *buffer);

typedef efi_status_t(EFIAPI *efi_wait_for_event_t)(
    efi_uint_t  number_of_events,
    void      **event,
    efi_uint_t *index
);

typedef efi_status_t(EFIAPI *efi_handle_protocol_t)(
    efi_handle_t     handle,
    struct efi_guid *protocol,
    void           **interface
);

typedef efi_status_t(EFIAPI *efi_image_unload_t)(efi_handle_t image_handle);

typedef efi_status_t(EFIAPI *efi_exit_boot_services_t)(
    efi_handle_t image_handle,
    efi_uint_t   map_key
);

typedef efi_status_t(EFIAPI *efi_set_watchdog_timer_t)(
    efi_uint_t timeout,
    efi_uint_t watchdog_code,
    efi_uint_t data_size,
    uint16_t  *watchdog_data
);

typedef efi_status_t(EFIAPI *efi_open_protocol_t)(
    efi_handle_t     handle,
    struct efi_guid *protocol,
    void           **interface,
    efi_handle_t     agent_handle,
    efi_handle_t     controller_handle,
    uint32_t         attributes
);

typedef efi_status_t(EFIAPI *efi_close_protocol_t)(
    efi_handle_t     handle,
    struct efi_guid *protocol,
    efi_handle_t     agent_handle,
    efi_handle_t     controller_handle
);

enum efi_local_search_type
{
    ALL_HANDLES,
    BY_REGISTER_NOTIFY,
    BY_PROTOCOL
};

typedef efi_status_t(EFIAPI *efi_locate_handle_buffer_t)(
    enum efi_local_search_type search_type,
    struct efi_guid           *protocol,
    void                      *search_key,
    efi_uint_t                *no_handles,
    efi_handle_t             **buffer
);

typedef efi_status_t(EFIAPI *efi_locate_protocol_t)(
    struct efi_guid *protocol,
    void            *registration,
    void           **interface
);


typedef void(EFIAPI *efi_copy_mem_t)(
    void      *destination,
    void      *source,
    efi_uint_t length
);

typedef void(EFIAPI
                 *efi_set_mem_t)(void *buffer, efi_uint_t size, uint8_t value);

typedef void(EFIAPI *efi_reset_system_t)(
    enum efi_reset_type reset_type,
    efi_status_t        reset_status,
    efi_uint_t          data_size,
    void               *reset_data
);

/// Uefi Runtime services
struct efi_runtime_services
{
    uint8_t buf_rs1[24];

    //
    // Time Services
    //
    efi_uint_t buf_rs2[4];

    //
    // Virtual Memory Services
    //
    efi_uint_t buf_rs3[2];

    //
    // Variable Services
    //
    efi_uint_t buf_rs4[3];

    //
    // Miscellaneous Services
    //
    efi_uint_t         buf_rs5;
    efi_reset_system_t reset_system;
};

struct efi_boot_services
{
    char buf1[24];

    // Task Priority Services
    efi_uint_t buf2[2];

    // Memory Services
    efi_allocate_pages_t allocate_pages;
    efi_free_pages_t     free_pages;
    efi_get_memory_map_t get_memory_map;
    efi_allocate_pool_t  allocate_pool;
    efi_free_pool_t      free_pool;

    // Event & Timer Services
    efi_uint_t           buf4[2];
    efi_wait_for_event_t wait_for_event;
    efi_uint_t           buf4_2[3];

    // Protocol Handler Services
    efi_uint_t            buf5_1[3];
    efi_handle_protocol_t handle_protocol;
    efi_uint_t            buf5_2[5];

    // Image Services
    efi_uint_t               buf6_1[3];
    efi_image_unload_t       unload_image;
    efi_exit_boot_services_t exit_boot_services;
    // Miscellaneous Services
    efi_uint_t               buf7[2];
    efi_set_watchdog_timer_t set_watchdog_timer;

    // DriverSupport Services
    efi_uint_t buf8[2];

    // Open and Close Protocol Services
    efi_open_protocol_t  open_protocol;
    efi_close_protocol_t close_protocol;
    efi_uint_t           buf9[1];

    // Library Services
    efi_uint_t                 buf10[1];
    efi_locate_handle_buffer_t locate_handle_buffer;
    efi_locate_protocol_t      locate_protocol;
    efi_uint_t                 buf_10_2[2];
    // 32-bit CRC Services
    efi_uint_t                 buf11;

    // Miscellaneous Services
    efi_copy_mem_t copy_mem;
    efi_set_mem_t  set_mem;
    efi_uint_t     buf12;
};

struct efi_configuration_table
{
    struct efi_guid vendor_guid;
    void           *vendor_table;
};

struct efi_system_table
{
    uint8_t                                 buf1[44];
    struct efi_simple_text_input_protocol  *con_in;
    efi_uint_t                              buf2;
    struct efi_simple_text_output_protocol *con_out;
    unsigned long long                      buf3[2];
    struct efi_runtime_services            *runtime_services;
    struct efi_boot_services               *boot_services;
    efi_uint_t                              number_of_table_entries;
    struct efi_configuration_table         *configuration_table;
};

#endif /* __EFI_SPEC_H__ */