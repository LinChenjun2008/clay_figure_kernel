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

// kernel
#include <base.h>

#include <asm/mem/page.h>

#include <config.h>
#include <elf.h>
#include <ramfs.h>
#include <std/stdarg.h>

#define KERNEL_TEXT_BASE 0xffffffff80000000

// acpi.c
efi_status_t read_acpi_tables(struct boot_info *boot_info);

// elf.c
#include <elf_util.h>

// file.c
efi_status_t read_file(
    char16_t               *file_name,
    efi_physical_address_t *file_buffer_base,
    efi_uint_t             *file_size
);

// main.c
efi_status_t EFIAPI
efi_main(efi_handle_t in_image_handle, efi_system_table_t *in_system_table);

// memory.c
efi_status_t get_memory_map(struct memory_map *mmap);
efi_status_t create_page_table(void *pml4t);

// string.c
char16_t *strcpy16(char16_t *dst, const char16_t *src);
char16_t *strncpy16(char16_t *dst, const char16_t *src, size_t n);
int       strcmp16(const char16_t *str1, const char16_t *str2);
int       strncmp16(const char16_t *str1, const char16_t *str2, size_t n);
size_t    strlen16(const char16_t *str);
char16_t *strchr16(const char16_t *str, char16_t ch);
char16_t *strrchr16(const char16_t *str, char16_t ch);
char16_t *strcat16(char16_t *dst, char16_t *src);

// utils.c
int       vsprintf(char16_t *buf, const char16_t *fmt, va_list ap);
int       sprintf(char16_t *buf, const char16_t *fmt, ...);
int       printf(const char16_t *fmt, ...);
char16_t *char_to_char16(char *ch, char16_t *in_ch16);
int       compare_guid(struct efi_guid *guid1, struct efi_guid *guid2);

// video.c
efi_status_t set_video_mode(void);

#endif /* __BOOTLOADER_H__ */