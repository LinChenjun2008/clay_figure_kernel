// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include "bootloader.h"
#include <std/string.h>

efi_status_t set_video_mode(void)
{
    char     video_config[64];
    char16_t video_config_ch16[64];
    size_t   video_config_len = 64;
    read_config("VIDEO", video_config, &video_config_len);
    char_to_char16(video_config, video_config_ch16);

    char16_t                                current_mode[64];
    efi_uint_t                              size_of_info = 0;
    efi_graphcis_output_mode_information_t *mode_info    = NULL;
    efi_uint_t                              i;
    for (i = 0; i < gop->mode->max_mode; i++)
    {
        gop->query_mode(gop, i, &size_of_info, &mode_info);
        sprintf(
            current_mode,
            L"%dx%d",
            mode_info->horizontal_resolution,
            mode_info->vertical_resolution
        );
        if (strcmp16(video_config_ch16, current_mode) == 0)
        {
            gop->set_mode(gop, i);
        }
    }
    return EFI_SUCCESS;
}