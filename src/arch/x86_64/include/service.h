// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __SERVICE_H__
#define __SERVICE_H__

#define SERVICE_ID_BASE 0x80000000UL

#define SERVICES 5

#define TICK    SERVICE_ID_BASE
#define MM      SERVICE_ID_BASE + 1
#define VIEW    SERVICE_ID_BASE + 2
#define USB_SRV SERVICE_ID_BASE + 3
#define KBD_SRV SERVICE_ID_BASE + 4

enum
{
    TICK_NO = 0x00000000,
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
    MM_NO = 0x00000000,

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
     * @return Error code (m1.i1), 0x00000000 if success.
    */
    MM_READ_PROG_ADDR,

    MM_START_PROG,
    MM_EXIT,
};

enum
{
    VIEW_NO = 0x00000000,

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

PUBLIC bool  is_service_id(uint32_t sid);
PUBLIC pid_t service_id_to_pid(uint32_t sid);
PUBLIC void  service_init(void);

PUBLIC void tick_main(void);
PUBLIC void mm_main(void);
PUBLIC void view_main(void);
PUBLIC void usb_main(void);
PUBLIC void keyboard_main(void);

#endif