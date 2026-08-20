// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <bootloader.h>

struct efi_system_table             *system_table;
struct efi_boot_services            *boot_services;
struct efi_graphics_output_protocol *gop;
efi_handle_t                         image_handle;

struct efi_guid efi_graphics_output_protocol_guid =
    EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
struct efi_guid efi_loaded_image_protocol_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
struct efi_guid efi_simple_file_system_protocol_guid =
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
struct efi_guid efi_file_info_guid  = EFI_FILE_INFO_ID;
struct efi_guid efi_acpi_table_guid = EFI_ACPI_TABLE_GUID;

efi_status_t EFIAPI
efi_main(efi_handle_t in_image_handle, struct efi_system_table *in_system_table)
{
    efi_status_t status = EFI_SUCCESS;

    image_handle = in_image_handle;
    system_table = in_system_table;

    boot_services = system_table->boot_services;
    boot_services->locate_protocol(
        &efi_graphics_output_protocol_guid, NULL, (void **)&gop
    );

    // Disable watch dog timer
    boot_services->set_watchdog_timer(0, 0, 0, NULL);

    // Clear screen
    system_table->con_out->clear_screen(system_table->con_out);

    printf(L"Starting...\r\n");

    // Prepare system info
    struct system_info *system_info = prepare_system_info();
    struct boot_info   *boot_info   = system_info->boot_info;

    // Read kernel.
    efi_physical_address_t sys_addr;
    efi_uint_t             sys_size;
    read_file(L"kernel\\system", &sys_addr, &sys_size);
    printf(L"Read kernel/system: address %p,size=%d.\r\n", sys_addr, sys_size);
    uintptr_t physical_base = 0x100000;
    uintptr_t relocate_base = 0xffffffff80000000;
    uintptr_t entry;
    load_segment(sys_addr, &physical_base, &relocate_base, &entry);
    printf(
        L"Physical: %p,Relocate: %p,Entry: %p.\r\n",
        physical_base,
        relocate_base,
        entry
    );
    boot_info->relocate_base = relocate_base;

    // Allocate kernel stack (4kib)
    efi_physical_address_t kstack;
    status = boot_services->allocate_pages(
        EFI_ALLOCATE_ANY_PAGES, EFI_LOADER_DATA, 1, &kstack
    );
    if (EFI_ERROR(status))
    {
        printf(
            L"boot_services->allocate_pages(kstack): ERROR(%d).\n\r", status
        );
        return status;
    }
    boot_info->stack_base  = kstack;
    boot_info->stack_pages = 1;
    printf(
        L"stack: %p - %p.\r\n",
        kstack,
        kstack + boot_info->stack_pages * PG_SIZE
    );

    // acpi table
    read_acpi_tables(boot_info);

    // Video mode
    struct graphic_info *graphic_info = &boot_info->graphic_info;
    struct efi_graphcis_output_mode_information *mode_info = gop->mode->info;

    graphic_info->frame_buffer_base     = gop->mode->frame_buffer_base;
    graphic_info->horizontal_resolution = mode_info->horizontal_resolution;
    graphic_info->vertical_resolution   = mode_info->vertical_resolution;
    graphic_info->pixel_per_scanline    = mode_info->pixels_per_scan_line;
    printf(
        L"Video: %dx%d ppsl: %d.\r\n",
        graphic_info->horizontal_resolution,
        graphic_info->vertical_resolution,
        graphic_info->pixel_per_scanline
    );
    printf(L"Video: frame buffer: %p.\r\n", graphic_info->frame_buffer_base);

    // Create page table
    uintptr_t page_table_pos;
    status = create_page_table(&page_table_pos);
    if (EFI_ERROR(status))
    {
        printf(L"create_page_table: ERROR(%d).\n\r", status);
    }
    boot_info->page_table_pos = (void *)page_table_pos;
    printf(L"Page table: %p.\r\n", boot_info->page_table_pos);

    // Init page_mgr
    boot_info->memory_map.map_size           = 4096 * 4;
    boot_info->memory_map.buffer             = NULL;
    boot_info->memory_map.map_key            = 0;
    boot_info->memory_map.descriptor_size    = 0;
    boot_info->memory_map.descriptor_version = 0;

    status = get_memory_map(&boot_info->memory_map);
    if (EFI_ERROR(status))
    {
        printf(L"get_memory_map: ERROR(%d).\n\r");
        return status;
    };
    init_page_mgr(system_info);

    // Get memory map (final).
    printf(L"Get memory map & exit boot service.\r\n");
    boot_info->memory_map.map_size           = 4096 * 4;
    boot_info->memory_map.buffer             = NULL;
    boot_info->memory_map.map_key            = 0;
    boot_info->memory_map.descriptor_size    = 0;
    boot_info->memory_map.descriptor_version = 0;

    status = get_memory_map(&boot_info->memory_map);
    if (EFI_ERROR(status))
    {
        printf(L"get_memory_map: ERROR(%d).\n\r");
        return status;
    }

    // Exit boot service
    status = boot_services->exit_boot_services(
        image_handle, boot_info->memory_map.map_key
    );
    if (EFI_ERROR(status))
    {
        return status;
    }

    preprocess_system_info(system_info);
    int(SYSV_ABI * kernel)(struct system_info *, uintptr_t) = (void *)(entry);

    status = kernel(system_info, page_table_pos);

    while (1);
    return status;
}

struct system_info *prepare_system_info(void)
{
    // Prepare system info
    struct system_info *sys_info = efi_malloc(sizeof(*sys_info));
    if (sys_info == NULL)
    {
        printf(L"cannot alloc memory for system_info.\n\r");
        return NULL;
    }
    boot_services->set_mem(sys_info, sizeof(*sys_info), 0);

    // Prepare boot info
    size_t boot_info_size = sizeof(*sys_info->boot_info);
    sys_info->boot_info   = efi_malloc(boot_info_size);
    if (sys_info->boot_info == NULL)
    {
        printf(L"cannot alloc memory for boot_info.\n\r");
        return NULL;
    }
    boot_services->set_mem(sys_info->boot_info, boot_info_size, 0);

    // pg_allocator
    sys_info->page_mgr = efi_malloc(sizeof(*sys_info->page_mgr));
    if (sys_info->page_mgr == NULL)
    {
        printf(L"cannot alloc memory for pg_mgr.\n\r");
        return NULL;
    }

    // cpus
    struct cpu *cpu = efi_malloc(sizeof(sys_info->cpu[0]));
    sys_info->cpu   = cpu;

    // task_mgr
    sys_info->task_mgr = efi_malloc(sizeof(*sys_info->task_mgr));
    if (sys_info->task_mgr == NULL)
    {
        printf(L"cannot alloc memory for task_mgr.\n\r");
        return NULL;
    }
    return sys_info;
}

void preprocess_system_info(struct system_info *system_info)
{
    struct boot_info *boot_info = system_info->boot_info;
    struct page_mgr  *page_mgr  = system_info->page_mgr;
    struct cpu       *cpu       = system_info->cpu;
    struct task_mgr  *task_mgr  = system_info->task_mgr;

    system_info->boot_info = PHYS_TO_VIRT(boot_info);
    system_info->page_mgr  = PHYS_TO_VIRT(page_mgr);
    system_info->cpu       = PHYS_TO_VIRT(cpu);
    system_info->task_mgr  = PHYS_TO_VIRT(task_mgr);

    task_mgr->system_info = PHYS_TO_VIRT(system_info);
    cpu->task_mgr         = PHYS_TO_VIRT(task_mgr);
    return;
}
