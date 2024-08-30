/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.
Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/global.h>
#include <task/task.h>
#include <service.h>

PRIVATE pid_t service_pid_table[SERVICES];

PUBLIC bool is_service_id(uint32_t sid)
{
    return sid >= SERVICE_ID_BASE && sid < SERVICE_ID_BASE + SERVICES;
}

PUBLIC pid_t service_id_to_pid(uint32_t sid)
{
    if (is_service_id(sid))
    {
        return service_pid_table[sid - SERVICE_ID_BASE];
    }
    return MAX_TASK;
}

PUBLIC void tick_main();
PUBLIC void mm_main();
PUBLIC void view_main();
PUBLIC void usb_main();
PUBLIC void keyboard_main();

PUBLIC void service_init()
{
    service_pid_table[TICK    - SERVICE_ID_BASE] = prog_execute(
                                                    "TICK",
                                                    SERVICE_PRIORITY,
                                                    4096,
                                                    tick_main)->pid;
    service_pid_table[MM      - SERVICE_ID_BASE] = task_start(
                                                    "MM",
                                                    SERVICE_PRIORITY,
                                                    4096,
                                                    mm_main,
                                                    0)->pid;
    service_pid_table[VIEW    - SERVICE_ID_BASE] = prog_execute(
                                                    "VIEW",
                                                    SERVICE_PRIORITY,
                                                    4096,
                                                    view_main)->pid;
    service_pid_table[USB_SRV - SERVICE_ID_BASE] = task_start(
                                                    "USB service",
                                                    SERVICE_PRIORITY,
                                                    4096,
                                                    usb_main,
                                                    0)->pid;
    service_pid_table[KBD_SRV - SERVICE_ID_BASE] = task_start(
                                                    "Keyboard service",
                                                    SERVICE_PRIORITY,
                                                    4096,
                                                    keyboard_main,
                                                    0)->pid;
    return;
}