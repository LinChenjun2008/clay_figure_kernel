// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <mem/page.h>
#include <panic.h>
#include <print.h>
#include <services.h>
#include <std/string.h>
#include <syscall/ipc.h>
#include <sysinfo.h>
#include <task.h>
#include <task/process.h>
#include <task/struct.h>
#include <user/services/vfs.h>

static char *print_progress(char *str, int width, int persent)
{
    int filled = persent * width / 100;
    int i;
    for (i = 0; i < width; i++)
    {
        if (i < filled)
            str[i] = '=';
        else if (i == filled)
            str[i] = '>';
        else
            str[i] = ' ';
    }
    str[i] = '\0';
    return str;
}

static void launch_vfs(struct system_info *system_info)
{
    struct task *task = NULL;

    task = process_execute("vfs", DEFAULT_PRIO, 1, 1, NULL);
    ASSERT(task != NULL);
    // 初始化vfs
    struct msg_head *msg = kallocate_a_page();
    msg->key             = 0;
    msg->type            = MSG_NORMAL;
    msg->header_legnth   = sizeof(*msg);

    struct vfs_ramfs_size *ramfs_size = (void *)msg;
    ramfs_size->size                  = system_info->boot_info->initramfs_size;
    msg->legnth                       = sizeof(*ramfs_size);

    int status = 0;
    status     = ipc_send(msg, task->pid);
    if (status < 0)
    {
        PERROR(status);
    }

    // read ramfs
    uintptr_t ramfs_base  = (uintptr_t)system_info->boot_info->initramfs;
    size_t    total_size  = system_info->boot_info->initramfs_size;
    ssize_t   remain_size = total_size;

    char str[32];
    while (remain_size >= 0)
    {
        struct vfs_ramfs_data *ramfs_data = (void *)msg;
        ramfs_data->head.header_legnth    = sizeof(ramfs_data->head);
        ramfs_data->head.legnth           = PG_SIZE;

        size_t copy_size = PG_SIZE - sizeof(*ramfs_data);
        void  *copy_src  = (void *)ramfs_base;
        void  *copy_dst  = (uint8_t *)ramfs_data->data;

        int progress = 100 - remain_size * 100 / total_size;
        print_progress(str, sizeof(str) / sizeof(str[0]) - 1, progress);
        printk("\r%s| %3d%% | address %p", str, progress, ramfs_base);

        memcpy(copy_dst, copy_src, copy_size);
        remain_size -= copy_size;
        // 结尾标记.
        if (remain_size <= 0)
        {
            ramfs_data->flag = 1;
        }
        status = ipc_send(msg, task->pid);
        if (status < 0)
        {
            PERROR(status);
        }
        ramfs_base += copy_size;
    }
    return;
}

void launch_services(struct system_info *sysinfo)
{
    printk("launch VFS service\n");
    launch_vfs(sysinfo);
    return;
}