// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#include <efi.h>
#include <efi/acpi.h>
#include <efi/protocol/graphics_output.h>
#include <efi/protocol/loaded_image.h>
#include <std/stdarg.h>

efi_status_t EFIAPI efi_main(
    efi_handle_t             in_image_handle,
    struct efi_system_table *in_system_table
);
struct system_info *prepare_system_info(void);
void                preprocess_system_info(struct system_info *system_info);
void                init_task_mgr(struct system_info *system_info);

// acpi.c
efi_status_t read_acpi_tables(struct boot_info *boot_info);

// elf.c
int load_segment(
    void      *file,
    uintptr_t *physical_base,
    uintptr_t *relocate_base,
    uintptr_t *entry
);

// file.c
efi_status_t
read_file(char16_t *file_name, void **file_buffer_base, efi_uint_t *file_size);
void *ramfs_open(void *fs, const char *filename);

// guid.c
int compare_guid(struct efi_guid *guid1, struct efi_guid *guid2);

// memory.c
#include <asm/page.h>

void        *efi_malloc(size_t size);
efi_status_t get_memory_map(struct memory_map *memmap);
efi_status_t create_page_table(void *pg_dir);
efi_status_t init_page_mgr(struct system_info *system_info);

// std.c
int       vsprintf(char16_t *buf, const char16_t *fmt, va_list ap);
int       sprintf(char16_t *buf, const char16_t *fmt, ...);
int       printf(const char16_t *fmt, ...);
char16_t *char_to_char16(char *ch, char16_t *in_ch16);
int       strncmp(const char *str1, const char *str2, size_t n);

#endif /* __BOOTLOADER_H__ */