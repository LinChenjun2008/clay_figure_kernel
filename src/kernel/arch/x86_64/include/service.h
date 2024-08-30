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

#ifndef __SERVICE_H__
#define __SERVICE_H__

enum
{
    TICK_NO = 0x80000000,
    /**
     * GET_TICKS
     * return: ticks(m3.l1)
    */
    TICK_GET_TICKS,

    /**
     * SLEEP
     * @param msecond (m3.l1)
     */
    TICK_SLEEP,
};

enum
{
    MM_NO = 0x80000000,

    /**
     * MM_ALLOCATE_PAGE
     * @param count The number of page will be allocated (m1.i1).
     * @return The base address of the allocated page (m2.p1).
    */
    MM_ALLOCATE_PAGE,

    /**
     * MM_FREE_PAGE
     * @param addr The base address of the page to be freed (m3.p1).
     * @param count The number of pages (m3.i1).
    */
    MM_FREE_PAGE,

    /**
     * MM_READ_PROG_ADDR
     * @param pid Read the address of this program (m3.i1).
     * @param addr The base address of the data (m3.p1).
     * @param size The number of bytes you want to read (m3.l1).
     * @param buffer The buffer to save the data you read (m3.p2).
     *               You must ensure that the buffer (range: [buffer , buffer + size]) is available.
     * @return Error code (m1.i1), 0x80000000 if success.
    */
    MM_READ_PROG_ADDR,

    MM_START_PROG,
    MM_EXIT,
};

enum
{
    VIEW_NO = 0x80000000,

    /**
     * VIEW_PUT_PIXEL
     * @param color m1.i1
     * @param x m1.i2
     * @param y m1.i3
    */
    VIEW_PUT_PIXEL,

    /**
     * VIEW_FILL
     * @param buffer m3.p1
     * @param buffer_szie m3.li
     * @param xszie m3.i1
     * @param yszie m3.i2
     * @param x m3.i3
     * @param y m3.i4
    */
    VIEW_FILL,
};

typedef struct service_task_s service_task_t;

PUBLIC bool is_service_id(uint32_t sid);
PUBLIC pid_t service_id_to_pid(uint32_t sid);
PUBLIC void service_init();

#endif