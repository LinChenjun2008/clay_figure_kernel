// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __EFI_SPEC_H__
#define __EFI_SPEC_H__

#include <efi/multiphase.h>
#include <efi/protocol/device_path.h>
#include <efi/protocol/simple_text_in.h>
#include <efi/protocol/simple_text_out.h>

// Runtime services

//
// Time Services
//

struct efi_time
{
    uint16_t year;   /* 1900 - 9999       */
    uint8_t  month;  /*    1 - 12         */
    uint8_t  day;    /*    1 - 31         */
    uint8_t  hour;   /*    0 - 23         */
    uint8_t  minute; /*    0 - 59         */
    uint8_t  second; /*    0 - 59         */
    uint8_t  pad1;
    uint32_t nanosecond; /*    0 - 999,999,999*/
    int16_t  time_zone;
    uint8_t  daylight;
    uint8_t  pad2;
};

struct efi_time_capabilities
{
    uint32_t resolution;
    uint32_t accuracy;
    boolen_t sets_to_zero;
};

typedef efi_status_t(EFIAPI *efi_get_time_t)(
    struct efi_time              *time,
    struct efi_time_capabilities *capabilities
);


typedef efi_status_t(EFIAPI *efi_set_time_t)(struct efi_time *time);

typedef efi_status_t(EFIAPI *efi_get_wakeup_time_t)(
    boolen_t        *enabled,
    boolen_t        *pending,
    struct efi_time *time
);

typedef efi_status_t(EFIAPI *efi_set_wakeup_time_t)(
    boolen_t         enable,
    struct efi_time *time
);

//
// Virtual Memory Services
//

struct efi_memory_descriptor
{
    uint32_t               type;
    efi_physical_address_t physical_start;
    efi_virtual_address_t  virtual_start;
    uint64_t               number_of_pages;
    uint64_t               attribute;
} ALIGNED(16);

typedef efi_status_t(EFIAPI *efi_set_virtual_address_map_t)(
    efi_uint_t                    memory_map_size,
    efi_uint_t                    descriptor_size,
    uint32_t                      descriptor_version,
    struct efi_memory_descriptor *virtual_map
);

typedef efi_status_t(EFIAPI *efi_convert_pointer_t)(
    efi_uint_t debug_disposition,
    void     **address
);

//
// Variable Services
//

typedef efi_status_t(EFIAPI *efi_get_variable_t)(
    char16_t        *variable_name,
    struct efi_guid *vendor_guid,
    uint32_t        *attributes,
    efi_uint_t      *data_size,
    void            *data
);

typedef efi_status_t(EFIAPI *efi_get_next_variable_name_t)(
    efi_uint_t      *variable_name_size,
    char16_t        *variable_name,
    struct efi_guid *vendor_guid
);

typedef efi_status_t(EFIAPI *efi_set_variable_t)(
    char16_t        *variable_name,
    struct efi_guid *vendor_guid,
    uint32_t         attributes,
    efi_uint_t       data_size,
    void            *data
);

//
// Miscellaneous Services
//

typedef efi_status_t(EFIAPI *efi_get_next_high_monotonic_count_t)(
    uint32_t *high_count
);

typedef void(EFIAPI *efi_reset_system_t)(
    enum efi_reset_type reset_type,
    efi_status_t        reset_status,
    efi_uint_t          data_size,
    void               *reset_data
);

//
// UEFI 2.0 Capsule Services
//

struct efi_capsule_header
{
    struct efi_guid capsule_guid;
    uint32_t        header_size;
    uint32_t        flags;
    uint32_t        capsule_image_size;
};

typedef efi_status_t(EFIAPI *efi_update_capsule_t)(
    struct efi_capsule_header **capsule_header_array,
    efi_uint_t                  capsule_count,
    efi_physical_address_t      scatter_gather_list
);

typedef efi_status_t(EFIAPI *efi_query_capsule_capabilities_t)(
    struct efi_capsule_header **capsule_header_array,
    efi_uint_t                  capsule_count,
    uint64_t                   *maximum_capsule_size,
    enum efi_reset_type        *reset_type
);

//
// Miscellaneous UEFI 2.0 Service
//

typedef efi_status_t(EFIAPI *efi_query_variable_info_t)(
    uint32_t  attributes,
    uint64_t *maximum_variable_storage_size,
    uint64_t *remaining_variable_storage_size,
    uint64_t *maximum_variable_size
);

// Boot services

//
// Task Priority Services
//

typedef efi_uint_t efi_tpl_t;

#define TPL_APPLICATION 4
#define TPL_CALLBACK    8
#define TPL_NOTIFY      16
#define TPL_HIGH_LEVEL  31

typedef efi_tpl_t(EFIAPI *efi_raise_tpl_t)(efi_tpl_t new_tpl);
typedef void(EFIAPI *efi_restore_tpl_t)(efi_tpl_t old_tpl);

//
// Memory Services
//

enum efi_allocate_type
{
    EFI_ALLOCATE_ANY_PAGES,
    EFI_ALLOCATE_MAX_ADDRESS,
    EFI_ALLOCATE_ADDRESS,
    EFI_MAX_ALLOCATE_TYPE
};

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

//
// Event & Timer Services
//

typedef void(EFIAPI *efi_event_notify_t)(efi_event_t *event, void *context);

typedef efi_status_t(EFIAPI *efi_create_event_t)(
    uint32_t           type,
    efi_tpl_t          notify_tpl,
    efi_event_notify_t notify_function,
    void              *notify_context,
    efi_event_t       *event
);

enum efi_timer_delay
{
    TIMER_CANCEL,
    TIMER_PERIODIC,
    TIMER_RELATIVE,
};

typedef efi_status_t(EFIAPI *efi_set_timer_t)(
    efi_event_t          event,
    enum efi_timer_delay type,
    uint64_t             trigger_time
);

typedef efi_status_t(EFIAPI *efi_wait_for_event_t)(
    efi_uint_t   number_of_events,
    efi_event_t *event,
    efi_uint_t  *index
);

typedef efi_status_t(EFIAPI *efi_signal_event_t)(efi_event_t event);

typedef efi_status_t(EFIAPI *efi_close_event_t)(efi_event_t event);

typedef efi_status_t(EFIAPI *efi_check_event_t)(efi_event_t event);

//
// Protocol Handler Services
//

enum efi_interface_type
{
    EFI_NATIVE_INTERFACE
};

typedef efi_status_t(EFIAPI *efi_install_protocol_interface_t)(
    efi_handle_t           *handle,
    struct efi_guid        *protocol,
    enum efi_interface_type interface_type,
    void                   *interface
);

typedef efi_status_t(EFIAPI *efi_reinstall_protocol_interface_t)(
    efi_handle_t     handle,
    struct efi_guid *protocol,
    void            *old_interface,
    void            *new_interface
);

typedef efi_status_t(EFIAPI *efi_uninstall_protocol_interface_t)(
    efi_handle_t     handle,
    struct efi_guid *protocol,
    void            *interface
);

typedef efi_status_t(EFIAPI *efi_handle_protocol_t)(
    efi_handle_t     handle,
    struct efi_guid *protocol,
    void           **interface
);

typedef efi_status_t(EFIAPI *efi_register_protocol_notify_t)(
    struct efi_guid *protocol,
    efi_event_t      event,
    void           **registration
);

enum efi_local_search_type
{
    ALL_HANDLES,
    BY_REGISTER_NOTIFY,
    BY_PROTOCOL
};

typedef efi_status_t(EFIAPI *efi_locate_handle_t)(
    enum efi_local_search_type search_type,
    struct efi_guid           *protocol,
    void                      *search_key,
    efi_uint_t                *buffer_size,
    efi_handle_t              *buffer
);

typedef efi_status_t(EFIAPI *efi_locate_device_path_t)(
    struct efi_guid                  *protocol,
    struct efi_device_path_protocol **device_path,
    efi_handle_t                     *device
);

typedef efi_status_t(EFIAPI *efi_install_configuration_table_t)(
    struct efi_guid *Guid,
    void            *table
);

//
// Image Services
//

typedef efi_status_t(EFIAPI *efi_image_load_t)(
    boolen_t                         boot_policy,
    efi_handle_t                     parent_image_handle,
    struct efi_device_path_protocol *device_path,
    void                            *source_buffer,
    efi_uint_t                       source_size,
    efi_handle_t                    *image_handle
);

typedef efi_status_t(EFIAPI *efi_image_start_t)(
    efi_handle_t image_handle,
    efi_uint_t  *exit_data_size,
    char16_t   **exit_data
);

typedef efi_status_t(EFIAPI *efi_exit_t)(
    efi_handle_t image_handle,
    efi_status_t exit_status,
    efi_uint_t  *exit_data_size,
    char16_t   **exit_data
);

typedef efi_status_t(EFIAPI *efi_image_unload_t)(efi_handle_t image_handle);

typedef efi_status_t(EFIAPI *efi_exit_boot_services_t)(
    efi_handle_t image_handle,
    efi_uint_t   map_key
);

//
// Miscellaneous Services
//

typedef efi_status_t(EFIAPI *efi_get_next_monotonic_count_t)(uint64_t *count);

typedef efi_status_t(EFIAPI *efi_stall_t)(efi_uint_t microseconds);

typedef efi_status_t(EFIAPI *efi_set_watchdog_timer_t)(
    efi_uint_t timeout,
    uint64_t   watchdog_code,
    efi_uint_t data_size,
    char16_t  *watchdog_data
);

//
// DriverSupport Services
//

typedef efi_status_t(EFIAPI *efi_connect_controller_t)(
    efi_handle_t                     controller_handle,
    efi_handle_t                    *driver_image_handle,
    struct efi_device_path_protocol *remaining_device_path,
    boolen_t                         recursive
);

typedef efi_status_t(EFIAPI *efi_disconnect_controller_t)(
    efi_handle_t controller_handle,
    efi_handle_t driver_image_handle,
    efi_handle_t child_handle
);

//
// Open and Close Protocol Services
//

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

struct efi_open_protocol_information_entry
{
    efi_handle_t agent_handle;
    efi_handle_t controller_handle;
    uint32_t     attributes;
    uint32_t     open_count;
};

typedef efi_status_t(EFIAPI *efi_open_protocol_information_t)(
    efi_handle_t                                 handle,
    struct efi_guid                             *protocol,
    struct efi_open_protocol_information_entry **entry_buffer,
    efi_uint_t                                  *entry_count
);

//
// Library Services
//

typedef efi_status_t(EFIAPI *efi_protocols_per_handle_t)(
    efi_handle_t       handle,
    struct efi_guid ***protocol_buffer,
    efi_uint_t        *protocol_buffer_count
);

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

typedef efi_status_t(EFIAPI *efi_install_multiple_protocol_interfaces_t)(
    efi_handle_t *handle,
    ...
);

typedef efi_status_t(EFIAPI *efi_uninstall_multiple_protocol_interfaces_t)(
    efi_handle_t *handle,
    ...
);

//
// 32-bit CRC Services
//

typedef efi_status_t(EFIAPI *efi_calculate_crc32_t)(
    void      *data,
    efi_uint_t data_size,
    uint32_t  *crc32
);

//
// Miscellaneous Services
//

typedef void(EFIAPI *efi_copy_mem_t)(
    void      *destination,
    void      *source,
    efi_uint_t length
);

typedef void(EFIAPI
                 *efi_set_mem_t)(void *buffer, efi_uint_t size, uint8_t value);

typedef efi_status_t(EFIAPI *efi_create_event_ex_t)(
    uint32_t               type,
    efi_tpl_t              notify_tpl,
    efi_event_notify_t     notify_function,
    const void            *context,
    const struct efi_guid *event_group,
    efi_event_t           *event
);

struct efi_table_header
{
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
};

/// Uefi Runtime services
struct efi_runtime_services
{
    struct efi_table_header hdr;

    //
    // Time Services
    //
    efi_get_time_t        get_time;
    efi_set_time_t        set_time;
    efi_get_wakeup_time_t get_wakeup_time;
    efi_set_wakeup_time_t set_wakeup_time;

    //
    // Virtual Memory Services
    //
    efi_set_virtual_address_map_t set_virtual_address_map;
    efi_convert_pointer_t         convert_pointer;

    //
    // Variable Services
    //
    efi_get_variable_t           get_variable;
    efi_get_next_variable_name_t get_next_variable_name;
    efi_set_variable_t           set_variable;

    //
    // Miscellaneous Services
    //
    efi_get_next_high_monotonic_count_t get_next_high_monotonic_count;
    efi_reset_system_t                  reset_system;

    //
    // UEFI 2.0 Capsule Services
    //
    efi_update_capsule_t             update_capsule;
    efi_query_capsule_capabilities_t query_capsule_capabilities;

    //
    // Miscellaneous UEFI 2.0 Service
    //
    efi_query_variable_info_t query_variable_info;
};

struct efi_boot_services
{
    struct efi_table_header hdr;

    //
    // Task Priority Services
    //
    efi_raise_tpl_t   raise_tpl;
    efi_restore_tpl_t restore_tpl;

    //
    // Memory Services
    //
    efi_allocate_pages_t allocate_pages;
    efi_free_pages_t     free_pages;
    efi_get_memory_map_t get_memory_map;
    efi_allocate_pool_t  allocate_pool;
    efi_free_pool_t      free_pool;

    //
    // Event & Timer Services
    //
    efi_create_event_t   create_event;
    efi_set_timer_t      set_timer;
    efi_wait_for_event_t wait_for_event;
    efi_signal_event_t   signal_event;
    efi_close_event_t    close_event;
    efi_check_event_t    check_event;

    //
    // Protocol Handler Services
    //
    efi_install_protocol_interface_t   install_protocol_interface;
    efi_reinstall_protocol_interface_t reinstall_protocol_interface;
    efi_uninstall_protocol_interface_t uninstall_protocol_interface;
    efi_handle_protocol_t              handle_protocol;
    void                              *reserved;
    efi_register_protocol_notify_t     register_protocol_notify;
    efi_locate_handle_t                locate_handle;
    efi_locate_device_path_t           locate_device_path;
    efi_install_configuration_table_t  install_configuration_table;

    //
    // Image Services
    //
    efi_image_load_t         load_image;
    efi_image_start_t        start_image;
    efi_exit_t               exit;
    efi_image_unload_t       unload_image;
    efi_exit_boot_services_t exit_boot_services;

    // Miscellaneous Services
    efi_get_next_monotonic_count_t get_next_monotonic_count;
    efi_stall_t                    stall;
    efi_set_watchdog_timer_t       set_watchdog_timer;

    // DriverSupport Services
    efi_connect_controller_t    connect_controller;
    efi_disconnect_controller_t disconnect_controller;

    // Open and Close Protocol Services
    efi_open_protocol_t             open_protocol;
    efi_close_protocol_t            close_protocol;
    efi_open_protocol_information_t open_protocol_information;

    // Library Services
    efi_protocols_per_handle_t protocols_per_handle;
    efi_locate_handle_buffer_t locate_handle_buffer;
    efi_locate_protocol_t      locate_protocol;
    efi_install_multiple_protocol_interfaces_t
        install_multiple_protocol_interfaces;
    efi_uninstall_multiple_protocol_interfaces_t
        uninstall_multiple_protocol_interfaces;

    // 32-bit CRC Services
    efi_calculate_crc32_t calculate_crc32;

    // Miscellaneous Services
    efi_copy_mem_t        copy_mem;
    efi_set_mem_t         set_mem;
    efi_create_event_ex_t create_event_ex;
};

struct efi_configuration_table
{
    struct efi_guid vendor_guid;
    void           *vendor_table;
};

#define EFI_SYSTEM_TABLE_SIGNATURE     0x5453595320494249
#define EFI_2_90_SYSTEM_TABLE_REVISION ((2 << 16) | (90))
#define EFI_2_80_SYSTEM_TABLE_REVISION ((2 << 16) | (80))
#define EFI_2_70_SYSTEM_TABLE_REVISION ((2 << 16) | (70))
#define EFI_2_60_SYSTEM_TABLE_REVISION ((2 << 16) | (60))
#define EFI_2_50_SYSTEM_TABLE_REVISION ((2 << 16) | (50))
#define EFI_2_40_SYSTEM_TABLE_REVISION ((2 << 16) | (40))
#define EFI_2_31_SYSTEM_TABLE_REVISION ((2 << 16) | (31))
#define EFI_2_30_SYSTEM_TABLE_REVISION ((2 << 16) | (30))
#define EFI_2_20_SYSTEM_TABLE_REVISION ((2 << 16) | (20))
#define EFI_2_10_SYSTEM_TABLE_REVISION ((2 << 16) | (10))
#define EFI_2_00_SYSTEM_TABLE_REVISION ((2 << 16) | (00))
#define EFI_1_10_SYSTEM_TABLE_REVISION ((1 << 16) | (10))
#define EFI_1_02_SYSTEM_TABLE_REVISION ((1 << 16) | (02))
#define EFI_SYSTEM_TABLE_REVISION      EFI_2_90_SYSTEM_TABLE_REVISION
#define EFI_SPECIFICATION_VERSION      EFI_SYSTEM_TABLE_REVISION

struct efi_system_table
{
    struct efi_table_header                 hdr;
    char16_t                               *firmware_vendor;
    uint32_t                                firmware_revision;
    efi_handle_t                            console_in_handle;
    struct efi_simple_text_input_protocol  *con_in;
    efi_handle_t                            console_out_handle;
    struct efi_simple_text_output_protocol *con_out;
    efi_handle_t                            standard_error_handle;
    struct efi_simple_text_output_protocol *stderr;
    struct efi_runtime_services            *runtime_services;
    struct efi_boot_services               *boot_services;
    efi_uint_t                              number_of_table_entries;
    struct efi_configuration_table         *configuration_table;
};

#endif /* __EFI_SPEC_H__ */