// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

/**
 * 将文件添加到ramfs中
 * 用法:
 * imgcopy -copy File1 Name1 ... -copy FileN NameN [> ImageName]
 * File* = 要添加的文件
 * Name* = 文件在ramfs中的名称
 * 生成的ramfs通过stdio输出,重定向到镜像文件中
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __RAMFS_TOOLS__
#include "../src/include/ramfs.h"

typedef struct
{
    char *origin_name;
    char *name;
} file_list_t;

void print_useage(char *argv[])
{
    fprintf(
        stderr,
        "Useage:\n"
        "\t%s -copy File1 Name1 ... -copy FileN NameN [> ImageName]\n",
        argv[0]
    );
    return;
}

int main(int argc, char *argv[])
{
    if (argc < 4 || argc % 3 != 1)
    {
        print_useage(argv);
        return -1;
    }
    ramfs_info_t ramfs_info;

    ramfs_info.files = (argc - 1) / 3;
    file_list_t *file_list;
    file_list = malloc(sizeof(*file_list) * ramfs_info.files);
    int i;
    for (i = 0; i < ramfs_info.files; i++)
    {
        int flag                 = i * 3 + 1;
        int file                 = flag + 1;
        int name                 = file + 1;
        file_list[i].origin_name = argv[file];
        file_list[i].name        = argv[name];
        if (strcmp("-copy", argv[flag]) != 0)
        {
            print_useage(argv);
            return 0;
        }
        if (strlen(file_list[i].name) > MAX_NAME_LEN)
        {
            fprintf(
                stderr, "Too long file name (must shorter than 63 byte).\n"
            );
            return -1;
        }
    }

    ramfs_info.magic = 0xffaaffaaffaaffaa;
    for (i = 0; i < sizeof(ramfs_info); i++)
    {
        putchar(((uint8_t *)&ramfs_info)[i]);
    }
    FILE *fp = NULL;
    for (i = 0; i < ramfs_info.files; i++)
    {
        fp = fopen(file_list[i].origin_name, "rb");
        if (fp == NULL)
        {
            fprintf(
                stderr,
                "Failed to open file %s (ignore).\n",
                file_list[i].origin_name
            );
            continue;
        }
        if (fseek(fp, 0, SEEK_END) != 0)
        {
            fprintf(stderr, "Failed to get file size.\n");
            return -1;
        }

        ramfs_file_meta_data_t fdata;
        fdata.file_size = ftell(fp);
        strncpy(fdata.file_name, file_list[i].name, MAX_NAME_LEN);
        int j;
        for (j = 0; j < sizeof(fdata); j++)
        {
            putchar(((uint8_t *)&fdata)[j]);
        }
        rewind(fp);
        int byte;
        for (byte = fgetc(fp); byte != EOF; byte = fgetc(fp))
        {
            putchar(byte);
        }
        fclose(fp);
    }
    printf("TRAILER!!!");
    return 0;
}