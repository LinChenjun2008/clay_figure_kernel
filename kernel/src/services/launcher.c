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

static void launch_vfs(struct system_info *system_info)
{
    struct task *task = NULL;

    task = process_execute("vfs", DEFAULT_PRIO, 1, 1, NULL);
    ASSERT(task != NULL);
    // 初始化vfs: 先告知ramfs镜像总大小
    struct msg_head msg;

    size_t total_size = system_info->boot_info->initramfs_size;

    msg.key  = 0;
    msg.type = MSG_NORMAL;
    msg.data = &total_size;
    msg.size = sizeof(total_size);

    int status = 0;
    status     = ipc_send(&msg, task->pid);
    if (status < 0)
    {
        PERROR(status);
    }

    // 发送ramfs镜像: head.data指向内核中的镜像, 由内核直接写入vfs的镜像缓冲区
    uintptr_t ramfs_base = (uintptr_t)system_info->boot_info->initramfs;
    size_t    offset     = 0;

    char str[32];
    while (offset < total_size)
    {
        size_t chunk = MIN(total_size - offset, PG_SIZE);

        int progress = (int)((offset + chunk) * 100 / total_size);
        print_progress(str, sizeof(str) / sizeof(str[0]) - 1, progress);
        printk("\r\t[%s]", str);

        msg.key  = 0;
        msg.type = MSG_NORMAL;
        msg.data = (void *)(ramfs_base + offset);
        msg.size = chunk;

        status = ipc_send(&msg, task->pid);
        if (status < 0)
        {
            PERROR(status);
        }
        offset += chunk;
    }
    printk("\n");
    return;
}

void launch_services(struct system_info *sysinfo)
{
    printk("launch VFS service.\n");
    launch_vfs(sysinfo);
    return;
}