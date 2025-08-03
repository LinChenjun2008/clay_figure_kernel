// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __SERVICE_H__
#define __SERVICE_H__

#include <kernel/syscall.h>

#define SERVICE_ID_BASE 0x80000000UL

#define SERVICES 5

#define TICK    SERVICE_ID_BASE
#define MM      SERVICE_ID_BASE + 1
#define VIEW    SERVICE_ID_BASE + 2
#define USB_SRV SERVICE_ID_BASE + 3
#define KBD_SRV SERVICE_ID_BASE + 4

typedef struct service_task_s service_task_t;

PUBLIC bool  is_service_id(uint32_t sid);
PUBLIC pid_t service_id_to_pid(uint32_t sid);
PUBLIC void  service_init(void);


PUBLIC void usb_main(void);
PUBLIC void keyboard_main(void);

////////////////////////////////////////////////////////////////////////////////
// Kernel services                                                            //
////////////////////////////////////////////////////////////////////////////////

// task
#define KERN_GET_PID        1
#define KERN_CREATE_PROCESS 2
#define KERN_EXIT           3

// memory
#define KERN_ALLOCATE_PAGE 4
#define KERN_FREE_PAGE     5
#define KERN_MMAP          6
#define KERN_MUNMAP        7
#define KERN_READ_PROC_MEM 8

PUBLIC syscall_status_t kernel_services(message_t *msg);

////////////////////////////////////////////////////////////////////////////////
// TICK                                                                       //
////////////////////////////////////////////////////////////////////////////////

#define TICK_GET_TICKS 1

PUBLIC void tick_main(void);

////////////////////////////////////////////////////////////////////////////////
// MM                                                                         //
////////////////////////////////////////////////////////////////////////////////

PUBLIC void mm_main(void);

////////////////////////////////////////////////////////////////////////////////
// VIEW                                                                       //
////////////////////////////////////////////////////////////////////////////////

#define VIEW_PUT_PIXEL 1
#define VIEW_FILL      2

PUBLIC void view_main(void);

#endif