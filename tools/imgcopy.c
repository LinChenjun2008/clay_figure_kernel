// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
#include <ramfs.h>

#define ALIGN_PAD(X, ALIGN) (((ALIGN) - ((X) & ((ALIGN) - 1))) & ((ALIGN) - 1))

static void print_help(char *name)
{
    printf("%s useage:\n", name);
    printf("\t-w [file] [name]\n");
    printf("\t-r [file] [name]\n");
    return;
}

static void write_file(char *file, char *name)
{
    FILE *fp = fopen(file, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "open '%s' failed.\n", file);
        return;
    }
    if (fseek(fp, 0, SEEK_END))
    {
        fprintf(stderr, "failed to get file size.\n");
        return;
    }
    size_t size = ftell(fp);
    rewind(fp);
    struct file_header header = { 0x55aa55aa, strlen(name), size };

    int i;
    // header
    for (i = 0; i < sizeof(header); i++)
    {
        putchar(((uint8_t *)&header)[i]);
    }

    // name
    for (i = 0; i < header.name_len; i++)
    {
        putchar(name[i]);
    }
    for (i = 0; i < ALIGN_PAD(header.name_len, 4); i++)
    {
        putchar(0);
    }

    // file
    while ((i = fgetc(fp)) != EOF)
    {
        putchar(i);
    }
    for (i = 0; i < ALIGN_PAD(header.file_size, 4); i++)
    {
        putchar(0);
    }

    fclose(fp);
    return;
}

int main(int argc, char *argv[])
{
    char *arg  = NULL;
    char *path = NULL;
    char *file = NULL;
    char *name = NULL;
    int   i;
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            path = argv[i];
        }
        arg = argv[i] + 1;
        if (strcmp("-help", arg) == 0)
        {
            print_help(argv[0]);
            return 0;
        }
    }
    char s[128];
    while (1)
    {
        if (fgets(s, sizeof(s), stdin) == NULL)
        {
            break;
        }
        size_t len = strlen(s);
        if (len > 0 && s[len - 1] == '\n')
        {
            s[len - 1] = '\0';
        }
        if (strncmp(path, s, strlen(path)) == 0)
        {
            file = s;
            name = s + strlen(path);
            write_file(file, name);
        }
    }
    name                      = "TRAILER!!!";
    struct file_header header = { 0x55aa55aa, strlen(name), 0 };
    for (i = 0; i < sizeof(header); i++)
    {
        putchar(((uint8_t *)&header)[i]);
    }
    for (i = 0; i < header.name_len; i++)
    {
        putchar(name[i]);
    }
    for (i = 0; i < ALIGN_PAD(header.name_len, 4); i++)
    {
        putchar(0);
    }
    return 0;
}
