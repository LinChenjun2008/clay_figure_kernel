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

#define KERN_EXIT          0
#define KERN_GET_PID       1
#define KERN_GET_PPID      2
#define KERN_CREATE_PROC   3
#define KERN_WAITPID       4
#define KERN_ALLOCATE_PAGE 5
#define KERN_FREE_PAGE     6
#define KERN_READ_TASK_MEM 7

#define KERN_SYSCALLS 8

// exit
#define IN_KERN_EXIT_STATUS 0

// get pid
#define OUT_KERN_GET_PID_PID 0

// get ppid
#define OUT_KERN_GET_PPID_PPID 0

// create process
#define IN_KERN_CREATE_PROC_NAME 0
#define IN_KERN_CREATE_PROC_PROC 1

#define OUT_KERN_CREATE_PROC_PID 0

// waitpid
#define IN_KERN_WAITPID_PID 0
#define IN_KERN_WAITPID_OPT 1

// IN_KERN_WAITPID_OPT
#define WNOHANG 1

#define OUT_KERN_WAITPID_STATUS 0
#define OUT_KERN_WAITPID_PID    1

// allocate page
#define OUT_KERN_ALLOCATE_PAGE_ADDR 0

// free page
#define IN_KERN_FREE_PAGE_ADDR 0

// read task mem
#define IN_KERN_READ_TASK_MEM_PID    0
#define IN_KERN_READ_TASK_MEM_ADDR   1
#define IN_KERN_READ_TASK_MEM_SIZE   2
#define IN_KERN_READ_TASK_MEM_BUFFER 3

PUBLIC syscall_status_t kernel_services(message_t *msg);

////////////////////////////////////////////////////////////////////////////////
// TICK                                                                       //
////////////////////////////////////////////////////////////////////////////////

#define TICK_GET_TICKS 1

#define OUT_TICK_GET_TICKS_TICKS 0

PUBLIC void tick_main(void);

////////////////////////////////////////////////////////////////////////////////
// MM                                                                         //
////////////////////////////////////////////////////////////////////////////////

PUBLIC void mm_main(void);

////////////////////////////////////////////////////////////////////////////////
// VIEW                                                                       //
////////////////////////////////////////////////////////////////////////////////

#define VIEW_FILL 1

#define IN_VIEW_FILL_BUFFER      0
#define IN_VIER_FILL_BUFFER_SIZE 1
#define IN_VIEW_FILL_XSIZE       2
#define IN_VIEW_FILL_YSIZE       3
#define IN_VIEW_FILL_X           4
#define IN_VIEW_FILL_Y           5

PUBLIC void view_main(void);

#endif